#include <Siv3D.hpp>
#include <HamFramework.hpp>
#include "Main.h"
#include "Loader.hpp"

#ifdef DEPLOY
String currentDocument(L"./doc/");
String configFile(L"./speedreader.ini");
String sampleDocument(L"./sample/");
#else
String currentDocument(L"../Speedreader/speedreader/doc/");
String configFile(L"../Speedreader/speedreader.ini");
String sampleDocument(L"../Speedreader/speedreader/sample/");
#endif

struct CommonData {

};

enum class sceneName {
	Loading,
	DisplayPages,
	DisplaySinglePage,
	DisplayBooks,
};


std::wstring displayOrder(L"LTR"); // Right to left(RTL): 0 LTR: 1
double joystickPointerSpeed = 1.0;
int drawingXOffset = 100;
double viewingPage = 0;
int displayMode = 1;
int autoplaySpeed = 0;
int debugTexureLoadingBenchmark;
int numPages;
XInput controller = XInput(0);
Vec2 pos;

void loadPDFConfig() {
	String filename = Format(currentDocument, L"config.ini");
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

void loadNewDocument(String path) {
	currentDocument = path;
	Profiler::EnableWarning(false); // 再読み込み時のテクスチャ破棄で警告されるので読み終わるまでdisable
	loadPDFConfig();
	viewingPage = 0;
	displayMode = 1;
	loader::loadPDF(currentDocument);
	numPages = loader::numPages;
}

void convertToDDS() {
	// TODO: make menu to call this
	for (auto i : step(100)) {
		Image(
			Format(L"C:/Users/nishio/Desktop/images/pages_{:04d}.png"_fmt, i + 1)
		).save(
			Format(L"C:/Users/nishio/Desktop/images/pages_{:04d}.dds"_fmt, i + 1)
		);
	}
}

int numDisplayingPages = 1;
class DisplayPages : public SceneManager<sceneName, CommonData>::Scene
{
public:
	void init() override
	{

	}

	void update() override
	{
		// 表示されているページ分だけまとめて進める
		if (controller.buttonA.clicked || Input::KeyDown.clicked) {
			viewingPage += numDisplayingPages;
			autoplaySpeed = 0;
		}
		if (controller.buttonB.clicked || Input::KeyUp.clicked) {
			viewingPage -= numDisplayingPages;
			autoplaySpeed = 0;
		}

		// Shift+Up/Downで1ページだけ前後する
		if ((Input::KeyDown + Input::KeyShift).clicked) {
			viewingPage++;
			autoplaySpeed = 0;
		}
		if ((Input::KeyUp + Input::KeyShift).clicked) {
			viewingPage--;
			autoplaySpeed = 0;
		}

		// Xボタンorクリックでそのページを通常表示
		if (controller.buttonX.clicked || Input::MouseL.clicked) {
			int ipage = static_cast<int>(viewingPage) % numPages;
			Texture t = loader::getPage(ipage);
			double h = static_cast<double>(t.height);
			double w = static_cast<double>(t.width);

			int screenHeight = Window::Height();
			double pageHeight = screenHeight, pageWidth = w / h * pageHeight;
			int numPageVertical = displayMode;
			int numPageHorizontal = (Window::Width() - drawingXOffset) * displayMode / pageWidth; // 右に余白を作らず描けるだけ描く
			pageHeight /= numPageVertical;
			pageWidth /= numPageVertical;

			if (Input::MouseL.clicked) {
				pos = Mouse::Pos();
			}
			int x = static_cast<int>((pos.x - drawingXOffset) / pageWidth);
			if (displayOrder != L"LTR") {
				x = numPageHorizontal - x - 1;
			}
			int y = static_cast<int>(pos.y / pageHeight);
			viewingPage = ipage + x + y * numPageHorizontal;
			displayMode = 1;
			Cursor::SetPos(0, 0);
			pos = { 0, 0 };
		}

	}

	void draw() const override
	{
		int ipage = static_cast<int>(viewingPage) % numPages;
		Texture t = loader::getPage(ipage);
		double h = static_cast<double>(t.height);
		double w = static_cast<double>(t.width);

		int screenHeight = Window::Height();
		double pageHeight = screenHeight, pageWidth = w / h * pageHeight;

		int numPageVertical = displayMode;
		//int numPageHorizontal = displayMode * horizontalMultiplier;
		int numPageHorizontal = (Window::Width() - drawingXOffset) * displayMode / pageWidth; // 右に余白を作らず描けるだけ描く
		pageHeight /= numPageVertical;
		pageWidth /= numPageVertical;
																							  // Tile mode
		if (displayOrder == L"LTR") {
			for (int y = 0; y < numPageVertical; y++) {
				for (int x = 0; x < numPageHorizontal; x++) {
					int i = ipage + y * numPageHorizontal + x;
					loader::getPage(i)
						.resize(pageWidth, pageHeight)
						.draw(drawingXOffset + pageWidth * x, pageHeight * y);

				}
			}
		}
		else {
			for (int y = 0; y < numPageVertical; y++) {
				for (int x = 0; x < numPageHorizontal; x++) {
					int i = ipage + y * numPageHorizontal + x;
					loader::getPage(i)
						.resize(pageWidth, pageHeight)
						.draw(drawingXOffset + pageWidth * (numPageHorizontal - x - 1), pageHeight * y);

				}
			}
		}
		numDisplayingPages = numPageVertical * numPageHorizontal;

	}
};

class DisplaySinglePage : public SceneManager<sceneName, CommonData>::Scene
{
public:
	void init() override
	{

	}

	void update() override
	{
		if (controller.buttonA.clicked || Input::KeyDown.clicked) viewingPage += 0.5;
		if (controller.buttonB.clicked || Input::KeyUp.clicked) viewingPage -= 0.5;
	}

	void draw() const override
	{
		// Zoom-in mode
		// いま2倍に拡大して半分ずつ表示する実装だが、もっとズームインしたり平行移動したりできた方がよいのではないか
		int ipage = static_cast<int>(viewingPage) % numPages;
		Texture t = loader::getPage(ipage);
		double h = static_cast<double>(t.height);
		double w = static_cast<double>(t.width);
		int screenHeight = Window::Height();
		double pageHeight = screenHeight, pageWidth = w / h * pageHeight;


		int scale = 2;
		if (static_cast<int>(viewingPage * 2) % 2 == 0) {
			t.resize(pageWidth * scale, pageHeight * scale).draw(drawingXOffset, 0);
		}
		else {
			t(0, h / 2, w, h / 2).resize(pageWidth * scale, pageHeight).draw(drawingXOffset, 0);
		}
	}
};


void Main()
{
	SceneManager<sceneName, CommonData> sceneManager;
	sceneManager.add<DisplayPages>(sceneName::DisplayPages);
	sceneManager.add<DisplaySinglePage>(sceneName::DisplaySinglePage);
	sceneManager.add<DisplayPages>(sceneName::DisplayPages);

	INIReader config(configFile);
	updateConfig(config);

	const Font font(30);
	const Font font10(10);

	loadPDFConfig();

	Window::SetTitle(L"Speedreader");
	Window::Resize(1300, 700);
	Window::ToUpperLeft();
	Window::SetStyle(WindowStyle::Sizeable);

	Cursor::SetPos(0, 0);

	Stopwatch stopwatch(true);
	loader::loadPDF(currentDocument);
	numPages = loader::numPages;

	// ESCでWindowを閉じない
	System::SetExitEvent(WindowEvent::CloseButton);

	controller = XInput(0);

	controller.setLeftTriggerDeadZone();
	controller.setRightTriggerDeadZone();
	controller.setLeftThumbDeadZone();
	controller.setRightThumbDeadZone();

	while (System::Update())
	{
		double invFPS = stopwatch.ms();
		stopwatch.restart();
		if (config.hasChanged()) updateConfig(config);

		if (Dragdrop::HasItems())
		{
			const Array<FilePath> items = Dragdrop::GetFilePaths();
			loadNewDocument(items[0]);
		}

		// まだ読み終わってなければ順次ロード
		loader::keepLoading();

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
			font10(L"自動再生: ", autoplaySpeed).draw(0, 0);
		}

		if (viewingPage < 0) viewingPage = 0;
		if (viewingPage > numPages - 1) viewingPage = numPages - 1;

		sceneManager.update();
		

		// 最初・最後のページで前後に移動した時に反対側に飛ぶべきかどうか、今disabled
		//if (page < 0) page += numPages;
		//if (page > numPages - 1) page = 0;
		if (viewingPage < 0) viewingPage = 0;
		if (viewingPage > numPages - 1) viewingPage = numPages - 1;

		int ipage = static_cast<int>(viewingPage) % numPages;

		if (controller.buttonLB.clicked || Input::KeyZ.clicked) {
			displayMode--;
			if (displayMode == 0) {
				sceneManager.changeScene(sceneName::DisplaySinglePage, 0, false);
			}
		}
		if (controller.buttonRB.clicked || Input::KeyC.clicked) {
			displayMode++;
			if (displayMode == 1) {
				sceneManager.changeScene(sceneName::DisplayPages, 0, false);
			}
		}

		Texture t = loader::getPage(ipage);
		double h = static_cast<double>(t.height);
		double w = static_cast<double>(t.width);
		//const int screenHeight = 640;
		int screenHeight = Window::Height();
		double pageHeight = screenHeight, pageWidth = w / h * pageHeight;

		int horizontalMultiplier = 2; // 見開きにするなら2、しないなら1
		if (h < w) horizontalMultiplier = 1;

		// draw progress bar
		int progressBarWidth = static_cast<int>(pageWidth * horizontalMultiplier);
		Rect(drawingXOffset, screenHeight + 10,
			progressBarWidth, 20).draw(Color(200));
		Rect(drawingXOffset, screenHeight + 12,
			progressBarWidth * (ipage + 1) / numPages, 16).draw(Color(100));
		font10(ipage + 1).draw(drawingXOffset, screenHeight + 12);

		const int32 w2 = font10(numPages).region().w;
		font10(numPages).draw(drawingXOffset + progressBarWidth - w2, screenHeight + 12);

		sceneManager.draw();


		// デモ用
		if (controller.buttonY.clicked || Input::KeyY.clicked) {
			loadNewDocument(sampleDocument);
		}

		// カーソル表示
		Circle(pos, 10).draw({ 255, 255, 0, 127 });
	}
}
