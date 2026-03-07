#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

namespace tme::sys::graphics {

class ShaderManager {

public:
	ShaderManager() = default;
	~ShaderManager() = default;

	void Init();

	// JSONファイルを読み込んで、記載されている全シェーダーをコンパイル＆キャッシュする
	bool LoadFromJSON(const std::wstring& jsonFilePath);

	// エイリアスでキャッシュからシェーダーを取り出す
	IDxcBlob* GetShader(const std::wstring& shaderName);

	// キャッシュにあれば即座に返し、無ければDXCでコンパイルして返す
	IDxcBlob* GetShader(const std::wstring& filePath,
		const std::wstring& entryPoint,
		const std::wstring& profile);

	// 特定のシェーダーを強制的に再コンパイルしてキャッシュを上書きする（ホットリロード用）
	// 成功なら true, 失敗なら false を返す
	bool ReloadShader(const std::wstring& filePath,
		const std::wstring& entryPoint,
		const std::wstring& profile);

	// 全てのキャッシュを破棄する
	void ClearCache();

private:
	// 実際にDXCを起動してコンパイルする処理
	Microsoft::WRL::ComPtr<IDxcBlob>
		CompileInternal(const std::wstring& filePath, const std::wstring& entryPoint,
		const std::wstring& profile);

	// キャッシュ用キーの生成
	std::wstring ParseCacheKey(const std::wstring& filePath,
		const std::wstring& entryPoint,
		const std::wstring& profile) const;

private:
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
	// キャッシュ本体:
	std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDxcBlob>>
		shaderCache_;
	// エイリアス名の対応表
	std::unordered_map<std::wstring, std::wstring> aliasMap_;
	bool isInitialized_ = false;
};

}