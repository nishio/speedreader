#include <Siv3D.hpp>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
#include "Main.h"

# include <ppl.h>
# include <ppltasks.h>


#ifdef DEPLOY
std::wstring currentDocument(L"./doc/");
std::wstring configFile(L"./speedreader.ini");
std::wstring sampleDocument(L"./sample/");
#else
std::wstring currentDocument(L"../Speedreader/speedreader/doc/");
std::wstring configFile(L"../Speedreader/speedreader.ini");
std::wstring sampleDocument(L"../Speedreader/speedreader/sample/");
#endif

const int maxPages = 1000;
Texture pageTextures[maxPages + 1];
std::wstring displayOrder(L"LTR"); // Right to left(RTL): 0 LTR: 1
double joystickPointerSpeed = 1.0;
int drawingXOffset = 100;
int numPages = maxPages;
int loadingPage = 0;
double viewingPage = 0;
int displayMode = 1;
int autoplaySpeed = 0;
int debugTexureLoadingBenchmark;

namespace s3d
{
	class TextureLoader // https://gist.github.com/Reputeless/780ac14c679c9f0a7d40e316171421ee
	{
	private:

		Array<FilePath> m_paths;

		Array<Image> m_images;

		Array<concurrency::task<void>> m_tasks;

		Array<Texture> m_textures;

		Array<bool> m_states;

		uint32 m_count_done = 0;

		bool m_isActive = false;

	public:

		TextureLoader() = default;

		TextureLoader(const Array<FilePath>& paths, bool startImmediately = false)
			: m_paths(paths)
			, m_images(paths.size())
			, m_textures(paths.size())
			, m_states(paths.size())
		{
			if (startImmediately)
			{
				start();
			}
		}

		~TextureLoader()
		{
			wait_all();
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
					[&path = m_paths[i], &image = m_images[i]]()
				{
					image = Image(path);
				}));
			}

			m_isActive = true;

			return;
		}

		void update(int32 maxTextureCreationPerFrame = 4)
		{
			if (!m_isActive || done())
			{
				return;
			}

			const int32 max = std::max(1, maxTextureCreationPerFrame);
			int32 n = 0;

			for (size_t i = 0; i < m_paths.size(); ++i)
			{
				if (!m_states[i] && m_tasks[i].is_done())
				{
					m_textures[i] = Texture(m_images[i]);

					m_images[i].release();

					m_states[i] = true;

					++m_count_done;

					if (++n >= maxTextureCreationPerFrame)
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

		void wait_all()
		{
			when_all(std::begin(m_tasks), std::end(m_tasks)).wait();
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

		bool getStates(size_t index) const
		{
			return m_states[index];
		}

		const Array<Texture>& getTextures() const
		{
			return m_textures;
		}

		const Texture& getTexture(size_t index) const
		{
			return m_textures[index];
		}
	};
}

Texture loadOneTexture(int fileid) {
	wchar filename[128];
	swprintf_s(filename, 128, L"%spages_%04d.png", currentDocument.c_str(), fileid + 1);
	pageTextures[fileid] = Texture(filename);
	return pageTextures[fileid];
}

void loadPDFConfig() {
	wchar filename[128];
	swprintf_s(filename, 128, L"%sconfig.ini", currentDocument.c_str());
	INIReader ini(filename);
	if (!ini)
	{
		INIWriter default_ini(filename);
		default_ini.write(L"displayOrder", L"LTR");
		ini = INIReader(filename);
	}
	displayOrder = ini.getOr<std::wstring>(L"displayOrder", L"LTR");
}

void updateConfig(INIReader	config) {
	config.reload();
	joystickPointerSpeed = config.getOr<double>(L"Controller.PointerSpeed", 1.0);
	drawingXOffset = config.getOr<int>(L"Drawing.XOffset", 100);
	debugTexureLoadingBenchmark = config.getOr<int>(L"Debug.TextureLoadingBenchmark", 0);
}

void loadNewDocument(const wchar* path) {
	currentDocument = path;
	loadingPage = 0;
	numPages = maxPages;
	Profiler::EnableWarning(false); // 再読み込み時のテクスチャ破棄で警告されるので読み終わるまでdisable
	loadPDFConfig();
	viewingPage = 0;
	displayMode = 1;
}

std::vector<Texture> ts;
void benchmark() {
	Stopwatch stopwatch;
	Profiler::EnableWarning(false);
	Println(L"runnning benchmark");

	stopwatch.start();
	for (auto i : step(100)) {
		/*
		Image(
			Format(L"C:/Users/nishio/Desktop/images/pages_{:04d}.png"_fmt, i + 1)
		).save(
			Format(L"C:/Users/nishio/Desktop/images/pages_{:04d}.dds"_fmt, i + 1)
		);
		*/ // 39752msec
		//ts.emplace_back(Format(L"C:/Users/nishio/Desktop/images/pages_{:04d}.dds"_fmt, i + 1));
		ts.emplace_back(Format(L"{}pages_{:04d}.png"_fmt, sampleDocument, i + 1));
	}
	Println(stopwatch.ms());
	pageTextures[0] = ts[0];
	pageTextures[1] = ts[1];
}

void benchmark2() {
	Stopwatch stopwatch(true);
	Window::Resize(1300, 700);
	Println(L"runnning benchmark2");
	Array<FilePath> paths;
	for (auto i : step(100))
		paths.push_back(Format(L"{}pages_{:04d}.png"_fmt, sampleDocument, i + 1));

	const bool startImmediately = true;

	// バックグラウンド & マルチスレッドで、paths で指定した画像を読み込み開始
	TextureLoader loader(paths, startImmediately);
	Stopwatch stopwatch2(true);
	while (System::Update())
	{
		if (loader.isActive() && !loader.done())
		{
			// ロードが完了した画像から順次 Texture を作成する。
			// 1 回の upadate で作成する Texture の最大数を少なくすると、フレームレートの低下を防げるが、
			// 最大数が 1　だと、100 枚の Texture を作成するには最低でも 100 フレーム必要になる。
			const int32 maxTextureCreationPerFrame = 1;
			loader.update(maxTextureCreationPerFrame);
		}

		if (!loader.done())
		{
			Print(L"{}/{} "_fmt, loader.num_loaded(), 
				stopwatch2.ms());
			stopwatch2.restart();

		}
		else if (stopwatch.isActive())
		{
			Print(L"ロード完了: {}ms"_fmt, stopwatch.ms());

			stopwatch.reset();
		}
	}
}



void Main()
{
	INIReader config(configFile);
	updateConfig(config);
	if (debugTexureLoadingBenchmark) {
		benchmark();
		benchmark2();
		//while (System::Update()) {};
		//return;
	}
	const Font font(30);
	const Font font10(10);
	// 見開きの描画用に最初の2枚を読んでおく
	for ( ; loadingPage < 2; loadingPage++) {
		loadOneTexture(loadingPage);
	}

	loadPDFConfig();

	Window::SetTitle(L"Speedreader");
	Window::Resize(1300, 700);
	Window::ToUpperLeft();
	Window::SetStyle(WindowStyle::Sizeable);

	Cursor::SetPos(0, 0);
	Vec2 pos = Mouse::Pos();


	Stopwatch stopwatch(true);
	/*
	Array<FilePath> paths;
	for (auto i : step(10))
		paths.push_back(Format(L"Images/{:0>4}.png"_fmt, i));

	const bool startImmediately = true;

	// バックグラウンド & マルチスレッドで、paths で指定した画像を読み込み開始
	TextureLoader loader(paths, startImmediately);
*/

	while (System::Update())
	{
		double invFPS = stopwatch.ms();
		stopwatch.restart();
		if (config.hasChanged()) updateConfig(config);

		if (Dragdrop::HasItems())
		{
			const Array<FilePath> items = Dragdrop::GetFilePaths();
			loadNewDocument(items[0].c_str());
		}

		// まだ読み終わってなければ順次ロード
		if (loadingPage < numPages) {
			if (!loadOneTexture(loadingPage)) {
				numPages = loadingPage;
				//Println(Format(L"Loaded:", numPages, L" pages"));
				//Profiler::EnableWarning(true);
			}
			else {
				loadingPage++;
			}
		}

		auto controller = XInput(0);

		controller.setLeftTriggerDeadZone();
		controller.setRightTriggerDeadZone();
		controller.setLeftThumbDeadZone();
		controller.setRightThumbDeadZone();

		pos += Vec2(
			controller.leftThumbX * joystickPointerSpeed * invFPS, 
			-controller.leftThumbY * joystickPointerSpeed * invFPS);
		//pos += Mouse::Delta();  // マウスも併用する場合はジョイスティックでの移動差分をマウスの入力と勘違いするので用修正
		if (pos.x < 0) pos.x = 0;
		if (pos.y < 0) pos.y = 0;
		if (pos.x > Window::Width()) pos.x = Window::Width();
		if (pos.y > Window::Height()) pos.y = Window::Height();


		viewingPage -= pow(controller.leftTrigger, 2);
		viewingPage += pow(controller.rightTrigger, 2);

		viewingPage += controller.rightThumbY / 5;  // slow

		// キーでのパラパラめくり// 早送り巻き戻しメタファー
		if (Input::KeyRight.clicked) {
			if (autoplaySpeed > 0) {
				autoplaySpeed *= 2;
			}
			else if (autoplaySpeed == 0) {
				autoplaySpeed = 1;
			}
			else {
				autoplaySpeed = 0;
			}
		}
		if (Input::KeyLeft.clicked) {
			if (autoplaySpeed < 0) {
				autoplaySpeed *= 2;
			}
			else if (autoplaySpeed == 0) {
				autoplaySpeed = -1;
			}
			else {
				autoplaySpeed = 0;
			}
		}
		if (autoplaySpeed) {
			viewingPage += autoplaySpeed * invFPS / 1000;
		}

		if (viewingPage < 0) viewingPage = 0;
		if (viewingPage > numPages - 1) viewingPage = numPages - 1;
		if (displayMode == 0) {
			if (controller.buttonA.clicked || Input::KeyDown.clicked) viewingPage += 0.5;
			if (controller.buttonB.clicked || Input::KeyUp.clicked) viewingPage -= 0.5;
		}
		else {
			if (controller.buttonA.clicked || Input::KeyDown.clicked) viewingPage++;
			if (controller.buttonB.clicked || Input::KeyUp.clicked) viewingPage--;
		}
		// 最初・最後のページで前後に移動した時に反対側に飛ぶべきかどうか、今disabled
		//if (page < 0) page += numPages;
		//if (page > numPages - 1) page = 0;
		if (viewingPage < 0) viewingPage = 0;
		if (viewingPage > numPages - 1) viewingPage = numPages - 1;

		int ipage = static_cast<int>(viewingPage) % numPages;

		if (controller.buttonLB.clicked || Input::KeyZ.clicked) {
			displayMode = (displayMode - 1);
			if (displayMode < 0) displayMode = 0;
		}
		if (controller.buttonRB.clicked || Input::KeyC.clicked) {
			displayMode = (displayMode + 1);
		}

		double h = static_cast<double>(pageTextures[ipage].height);
		double w = static_cast<double>(pageTextures[ipage].width);
		const int screenHeight = 640;
		double pageHeight = screenHeight, pageWidth = w / h * pageHeight;

		int horizontalMultiplier = 2; // 見開きにするなら2、しないなら1
		if (h < w) horizontalMultiplier = 1;

		// draw progress bar
		int progressBarWidth = static_cast<int>(pageWidth * horizontalMultiplier);
		Rect(drawingXOffset, screenHeight + 10,
			progressBarWidth, 20).draw(Color(200));
		Rect(drawingXOffset, screenHeight + 12,
			progressBarWidth * (ipage + 1) / loadingPage, 16).draw(Color(100));
		font10(ipage + 1).draw(drawingXOffset, screenHeight + 12);

		const int32 w2 = font10(loadingPage).region().w;
		font10(loadingPage).draw(drawingXOffset + progressBarWidth - w2, screenHeight + 12);

		if (displayMode == 0) {
			// 拡大表示モード
			int scale = 2;
			if (static_cast<int>(viewingPage * 2) % 2 == 0) {
				pageTextures[ipage].resize(pageWidth * scale, pageHeight * scale).draw(drawingXOffset, 0);
			}
			else {
				pageTextures[ipage](0, h / 2, w, h / 2).resize(pageWidth * scale, pageHeight).draw(drawingXOffset, 0);
			}
		}
		else if(displayMode > 0) {
			int numPageHorizontal = displayMode * horizontalMultiplier;
			int numPageVertical = displayMode;
			// 並べるモード
			// 見開き表示モード
			if (displayOrder == L"LTR") {
				pageHeight /= numPageVertical;
				pageWidth /= numPageVertical;
				for (int y = 0; y < numPageVertical; y++) {
					for (int x = 0; x < numPageHorizontal; x++) {
						int i = ipage + y * numPageHorizontal + x;
						if (i < numPages) {
							pageTextures[i]
								.resize(pageWidth, pageHeight)
								.draw(drawingXOffset + pageWidth * x, pageHeight * y);
						}
					}
				}
			}
			else {
				pageHeight /= numPageVertical;
				pageWidth /= numPageVertical;
				for (int y = 0; y < numPageVertical; y++) {
					for (int x = 0; x < numPageHorizontal; x++) {
						int i = ipage + y * numPageHorizontal + x;
						if (i < numPages) {
							pageTextures[i]
								.resize(pageWidth, pageHeight)
								.draw(drawingXOffset + pageWidth * (numPageHorizontal - x - 1), pageHeight * y);
						}
					}
				}
			}

			// Xボタンでそのページを通常表示
			if (controller.buttonX.clicked || Input::MouseL.clicked) {
				if (Input::MouseL.clicked) {
					pos = Mouse::Pos();
				}
				int x = static_cast<int>((pos.x - drawingXOffset) / pageWidth);
				if (displayOrder == L"RTL") {
					x = numPageHorizontal - x - 1;
				}
				int y = static_cast<int>(pos.y / pageHeight);
				viewingPage = ipage + x + y * numPageHorizontal;
				displayMode = 1;
				Cursor::SetPos(0, 0);
			}
		}

		// デモ用
		if (controller.buttonY.clicked) {
			loadNewDocument(L"C:/Users/nishio/Desktop/コーディングを支える技術 〜成り立ちから学ぶプログラミング作法 （WEB+DB PRESS plus） 西尾 泰和 264p_477415654X/");
		}

		// カーソル表示
		Circle(pos, 10).draw({ 255, 255, 0, 127 });
	}
}
