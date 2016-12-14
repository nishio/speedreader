//----------------------------------------------------------------------------------------
//
//	Siv3D Asset Loader (experimental)
//
//	Copyright (C) 2016 Ryo Suzuki
//
//	Licensed under the MIT License.
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files(the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions :
//	
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//	
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.
//
//----------------------------------------------------------------------------------------

# pragma once
# include <ppl.h>
# include <ppltasks.h>
# include <Siv3D.hpp>

namespace s3d
{
	namespace experimental
	{
		template <class AssetType, class AssetDataType>
		class AssetLoader
		{
		private:

			class AssetLoader_Impl;

			std::shared_ptr<AssetLoader_Impl> m_pImpl;

		public:

			AssetLoader();

			AssetLoader(const Array<FilePath>& paths, bool startImmediately);

			void start();

			void update(const int32 maxCreationPerFrame = 4);

			size_t num_loaded() const;

			size_t size() const;

			void waitAll();

			bool isActive();

			bool done() const;

			const Array<bool>& getStates() const;

			bool getState(const size_t index) const;

			const Array<AssetType>& getAssets() const;

			const AssetType& getAsset(const size_t index) const;
		};

		template <class AssetType, class AssetDataType>
		class AssetLoader<AssetType, AssetDataType>::AssetLoader_Impl
		{
		private:

			Array<FilePath> m_paths;

			Array<AssetDataType> m_assetData;

			Array<concurrency::task<void>> m_tasks;

			Array<AssetType> m_assets;

			Array<bool> m_states;

			uint32 m_count_done = 0;

			bool m_isActive = false;

		public:

			AssetLoader_Impl() = default;

			AssetLoader_Impl(const Array<FilePath>& paths, bool startImmediately)
				: m_paths(paths)
				, m_assetData(paths.size())
				, m_assets(paths.size())
				, m_states(paths.size())
			{
				if (startImmediately)
				{
					start();
				}
			}

			~AssetLoader_Impl()
			{
				waitAll();
			}

			void start()
			{
				if (m_isActive)
				{
					return;
				}

				for (size_t i = 0; i < m_paths.size(); ++i)
				{
					m_tasks.emplace_back(concurrency::create_task(
						[&path = m_paths[i], &assetData = m_assetData[i]]()
					{
						assetData = AssetDataType(path);
					}));
				}

				m_isActive = true;

				return;
			}

			void update(const int32 maxCreationPerFrame)
			{
				if (!m_isActive || done())
				{
					return;
				}

				const int32 max = std::max(1, maxCreationPerFrame);
				int32 n = 0;

				for (size_t i = 0; i < m_paths.size(); ++i)
				{
					if (!m_states[i] && m_tasks[i].is_done())
					{
						m_assets[i] = AssetType(m_assetData[i]);

						m_assetData[i].release();

						m_states[i] = true;

						++m_count_done;

						if (++n >= maxCreationPerFrame)
						{
							break;
						}
					}
				}
			}

			size_t num_loaded() const
			{
				return m_count_done;
			}

			size_t size() const
			{
				return m_paths.size();
			}

			void waitAll()
			{
				concurrency::when_all(std::begin(m_tasks), std::end(m_tasks)).wait();
			}

			bool isActive()
			{
				return m_isActive;
			}

			bool done() const
			{
				return num_loaded() == size();
			}

			const Array<bool>& getStates() const
			{
				return m_states;
			}

			const Array<AssetType>& getAssets() const
			{
				return m_assets;
			}
		};

		template <class AssetType, class AssetDataType>
		inline AssetLoader<AssetType, AssetDataType>::AssetLoader()
			: m_pImpl(std::make_shared<AssetLoader_Impl>()) {}

		template <class AssetType, class AssetDataType>
		inline AssetLoader<AssetType, AssetDataType>::AssetLoader(const Array<FilePath>& paths, bool startImmediately)
			: m_pImpl(std::make_shared<AssetLoader_Impl>(paths, startImmediately)) {}

		template <class AssetType, class AssetDataType>
		inline void AssetLoader<AssetType, AssetDataType>::start()
		{
			m_pImpl->start();
		}

		template <class AssetType, class AssetDataType>
		inline void AssetLoader<AssetType, AssetDataType>::update(const int32 maxCreationPerFrame)
		{
			m_pImpl->update(maxCreationPerFrame);
		}

		template <class AssetType, class AssetDataType>
		inline size_t AssetLoader<AssetType, AssetDataType>::num_loaded() const
		{
			return m_pImpl->num_loaded();
		}

		template <class AssetType, class AssetDataType>
		inline size_t AssetLoader<AssetType, AssetDataType>::size() const
		{
			return m_pImpl->size();
		}

		template <class AssetType, class AssetDataType>
		inline void AssetLoader<AssetType, AssetDataType>::waitAll()
		{
			m_pImpl->waitAll();
		}

		template <class AssetType, class AssetDataType>
		inline bool AssetLoader<AssetType, AssetDataType>::isActive()
		{
			return m_pImpl->isActive();
		}

		template <class AssetType, class AssetDataType>
		inline bool AssetLoader<AssetType, AssetDataType>::done() const
		{
			return m_pImpl->done();
		}

		template <class AssetType, class AssetDataType>
		inline const Array<bool>& AssetLoader<AssetType, AssetDataType>::getStates() const
		{
			return m_pImpl->getStates();
		}

		template <class AssetType, class AssetDataType>
		inline bool AssetLoader<AssetType, AssetDataType>::getState(const size_t index) const
		{
			return m_pImpl->getStates()[index];
		}

		template <class AssetType, class AssetDataType>
		inline const Array<AssetType>& AssetLoader<AssetType, AssetDataType>::getAssets() const
		{
			return m_pImpl->getAssets();
		}

		template <class AssetType, class AssetDataType>
		inline const AssetType& AssetLoader<AssetType, AssetDataType>::getAsset(const size_t index) const
		{
			return m_pImpl->getAssets()[index];
		}

		using TextureLoader = AssetLoader<Texture, Image>;
		using SoundLoader = AssetLoader<Sound, Wave>;
	}
}