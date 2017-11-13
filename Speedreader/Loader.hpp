#pragma once
#include <Siv3D.hpp>
#include <cstdio>
#include <string>
#include <vector>
#include "AssetLoader.hpp"

namespace loader
{
	bool useConcurrentLoader = false;
	s3d::experimental::TextureLoader loader;
	Texture nullPage;
	Array<Texture> frontPage(2);
	Array<FilePath> paths;
	uint32 numPages;
	uint32 loadingPage;
	s3d::PyFmtString fmt = L"{}page-{:03d}.png"_fmt;

	void loadPDF(String doc) {
		frontPage.resize(2);
		frontPage[0] = Texture(Format(fmt, doc, 1));
		frontPage[1] = Texture(Format(fmt, doc, 2));

		// TODO: iniにページ数が掛かれているならファイルシステムのチェックはいらない
		// TODO: ddsがあればそちらを読むようにする
		int i = 3;
		paths.clear();

		while (true) {
			auto path = Format(fmt, doc, i);
			if (!FileSystem::Exists(path)) break;
			paths.push_back(path);
			i++;
		}

		numPages = i - 1;

		// TODO: numPagesが0なら警告する
		if (useConcurrentLoader) {
			loader = s3d::experimental::TextureLoader(paths, true);
		}
		else {
			loadingPage = 0;
		}
	}

	void keepLoading() {
		if (useConcurrentLoader) {
			if (loader.isActive() && !loader.done())
			{
				// ロードが完了した画像から順次 Texture を作成する。
				// 1 回の upadate で作成する Texture の最大数を少なくすると、フレームレートの低下を防げるが、
				// 最大数が 1　だと、100 枚の Texture を作成するには最低でも 100 フレーム必要になる。
				const int32 maxTextureCreationPerFrame = 10;
				loader.update(maxTextureCreationPerFrame);
			}
		}
		else {
			if (loadingPage < paths.size()) {
				frontPage.push_back(Texture(paths[loadingPage]));
				loadingPage++;
			}
		}
	}

	const Texture& getPage(int i) {
		if (useConcurrentLoader) {
			if (i < 2) {
				return frontPage[i];
			}
			if (i >= numPages || !loader.getState(i - 2)) {
				return nullPage;
			}
			return loader.getAsset(i - 2);
		}
		else {
			if (i < frontPage.size()) {
				return frontPage[i];
			}
			else {
				return nullPage;
			}
		}
	}
}