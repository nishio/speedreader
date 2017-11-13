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

		// TODO: ini�Ƀy�[�W�����|����Ă���Ȃ�t�@�C���V�X�e���̃`�F�b�N�͂���Ȃ�
		// TODO: dds������΂������ǂނ悤�ɂ���
		int i = 3;
		paths.clear();

		while (true) {
			auto path = Format(fmt, doc, i);
			if (!FileSystem::Exists(path)) break;
			paths.push_back(path);
			i++;
		}

		numPages = i - 1;

		// TODO: numPages��0�Ȃ�x������
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
				// ���[�h�����������摜���珇�� Texture ���쐬����B
				// 1 ��� upadate �ō쐬���� Texture �̍ő吔�����Ȃ�����ƁA�t���[�����[�g�̒ቺ��h���邪�A
				// �ő吔�� 1�@���ƁA100 ���� Texture ���쐬����ɂ͍Œ�ł� 100 �t���[���K�v�ɂȂ�B
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