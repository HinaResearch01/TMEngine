#include "pch.h"
#include "ShaderManager.h"

using namespace Microsoft::WRL;
using json = nlohmann::json;
namespace tme::sys::graphics {

void ShaderManager::Init() {
	if (isInitialized_)
		return;

	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "Failed to create DXC Utils");
		return;
	}

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "Failed to create DXC Compiler");
		return;
	}

	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "Failed to create DXC Include Handler");
		return;
	}

	isInitialized_ = true;

	LOG_INFO("Graphics", "ShaderManager initialized successfully");
}

bool ShaderManager::LoadFromJSON(const std::wstring& jsonFilePath) {
	if (!isInitialized_)
		return false;

	// wstringのパスを通常stringに変換してファイルを開く
	std::ifstream jsonFile(tme::util::str::ToString(jsonFilePath));

	if (!jsonFile.is_open()) {
		LOG_ERROR("Graphics", "Shader JSON file not found Path: {}",
			tme::util::str::ToString(jsonFilePath));
		return false;
	}

	json j;

	try {
		jsonFile >> j;
	}
	catch (json::parse_error& e) {
		LOG_ERROR("Graphics", "Failed to parse JSON Error: {}", e.what());
		return false;
	}

	// JSONの中の "shaders" 配列をループ
	if (j.contains("shaders") && j["shaders"].is_array()) {
		for (const auto& shaderConfig : j["shaders"]) {

			// 必須項目があるかチェック
			if (!shaderConfig.contains("name") || !shaderConfig.contains("file") ||
				!shaderConfig.contains("entry") ||
				!shaderConfig.contains("profile")) {
				LOG_ERROR("Graphics",
					"Skipping incomplete shader config entry in JSON.");
				continue;
			}
			// std::string を std::wstring に変換
			std::wstring name = tme::util::str::ToWString(shaderConfig["name"].get<std::string>());
			std::wstring filew = tme::util::str::ToWString(shaderConfig["file"].get<std::string>());
			std::wstring entry = tme::util::str::ToWString(shaderConfig["entry"].get<std::string>());
			std::wstring profile = tme::util::str::ToWString(shaderConfig["profile"].get<std::string>());

			LOG_INFO("Graphics", "Pre-compiling shader from JSON Name: {}",
				shaderConfig["name"].get<std::string>());

			// コンパイルしてキャッシュにぶち込む
			IDxcBlob* blob = GetShader(filew, entry, profile);
			if (blob) {
				// 成功したら、エイリアスと長いキャッシュキーを紐付けておく
				std::wstring cacheKey = ParseCacheKey(filew, entry, profile);
				aliasMap_[name] = cacheKey;
			}
		}
	}

	LOG_INFO("Graphics", "--- Shader Preloading from JSON Completed ---");

	return true;
}

IDxcBlob* ShaderManager::GetShader(const std::wstring& shaderName) {
	auto itAlias = aliasMap_.find(shaderName);

	if (itAlias != aliasMap_.end()) {
		// あだ名から長いキーを見つけたら、いつものキャッシュ検索を走らせる
		auto itCache = shaderCache_.find(itAlias->second);
		if (itCache != shaderCache_.end()) {
			return itCache->second.Get();
		}
	}

	LOG_ERROR("Graphics", "Shader alias not found in preloaded cache Name: {}",
		tme::util::str::ToString(shaderName));

	return nullptr;
}

IDxcBlob* ShaderManager::GetShader(const std::wstring& filePath,
	const std::wstring& entryPoint,
	const std::wstring& profile) {
	if (!isInitialized_)
		return nullptr;

	std::wstring cacheKey = ParseCacheKey(filePath, entryPoint, profile);

	// キャッシュを検索
	auto it = shaderCache_.find(cacheKey);
	if (it != shaderCache_.end()) {
		// キャッシュヒット。即座に返す
		return it->second.Get();
	}

	// キャッシュに無かったので新規コンパイル
	auto compiledBlob = CompileInternal(filePath, entryPoint, profile);

	if (compiledBlob) {
		// 成功したらキャッシュに登録して返す
		shaderCache_[cacheKey] = compiledBlob;
		LOG_INFO("Graphics", "Shader cached successfully Key: {}",
			tme::util::str::ToString(cacheKey));
		return compiledBlob.Get();
	}

	// コンパイル失敗時は nullptr を返す
	return nullptr;
}

bool ShaderManager::ReloadShader(const std::wstring& filePath,
	const std::wstring& entryPoint,
	const std::wstring& profile) {
	if (!isInitialized_)
		return false;

	std::wstring cacheKey = ParseCacheKey(filePath, entryPoint, profile);

	LOG_INFO("Graphics", "Attempting to hot-reload shader Key: {}",
		tme::util::str::ToString(cacheKey));

	// 強制再コンパイル
	auto newBlob = CompileInternal(filePath, entryPoint, profile);

	if (newBlob) {
		// コンパイル成功。古いキャッシュを上書きする
		shaderCache_[cacheKey] = newBlob;
		LOG_INFO("Graphics", "Shader hot-reload SUCCESS");
		return true;
	}

	// 失敗。元のキャッシュは生かしたまま false を返す
	LOG_ERROR("Graphics", "Shader hot-reload FAILED. Retaining old cache.");
	return false;
}

void ShaderManager::ClearCache() {
	shaderCache_.clear();
	LOG_INFO("Graphics", "Shader cache cleared");
}

ComPtr<IDxcBlob> ShaderManager::CompileInternal(const std::wstring& filePath,
	const std::wstring& entryPoint,
	const std::wstring& profile) {
	// ファイル読み込み
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderText;

	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderText);

	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "CompileInternal: Failed to load file {}",
			tme::util::str::ToString(filePath));
		return nullptr;
	}

	// コンパイルオプション
	std::vector<LPCWSTR> arguments = {
		filePath.c_str(), L"-E",  entryPoint.c_str(), L"-T",
		profile.c_str(),  L"-Zi", L"-Qembed_debug",   L"-Od",
		L"-Zpr" };

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = shaderText->GetBufferPointer();
	sourceBuffer.Size = shaderText->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	// コンパイル実行
	Microsoft::WRL::ComPtr<IDxcResult> compileResult;
	hr = dxcCompiler_->Compile(&sourceBuffer, arguments.data(),
		(uint32_t)arguments.size(), includeHandler_.Get(),
		IID_PPV_ARGS(&compileResult));

	if (FAILED(hr) || !compileResult) {
		return nullptr;
	}

	// エラー＆警告出力
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
	compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

	if (errors && errors->GetStringLength() > 0) {
		std::string errorStr = errors->GetStringPointer();
		LOG_ERROR("Graphics", "DXC Compile Log:\n{}", errorStr);
	}

	// ステータスチェック
	HRESULT status;
	compileResult->GetStatus(&status);
	if (FAILED(status)) {
		return nullptr;
	}

	// バイトコード取得
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBytecode;
	hr = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBytecode),
		nullptr);

	if (FAILED(hr)) {
		return nullptr;
	}

	return shaderBytecode;
}

std::wstring ShaderManager::ParseCacheKey(const std::wstring& filePath,
	const std::wstring& entryPoint,
	const std::wstring& profile) const {
	return filePath + L"||" + entryPoint + L"||" + profile;
}

}