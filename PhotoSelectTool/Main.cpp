# include <Siv3D.hpp> // Siv3D v0.6.15

void Main() {
    Scene::SetBackground(Palette::Black);
    Size window_size = Size(2000, 1334);
    Window::Resize(window_size);
    const Font font{ FontMethod::MSDF, 48, Typeface::Bold };

    TextAreaEditState text_area[2];

    bool dir_loaded = false;

    while (System::Update()) {

        if (dir_loaded) {

        } else {
            font(U"Input Directory: ").draw(30, Arg::topRight(400, 50), Palette::White);
            SimpleGUI::TextArea(text_area[0], Vec2{400, 50}, SizeF{1000, 30}, SimpleGUI::PreferredTextAreaMaxChars);
            font(U"Output Directory: ").draw(30, Arg::topRight(400, 100), Palette::White);
            SimpleGUI::TextArea(text_area[1], Vec2{400, 100}, SizeF{1000, 30}, SimpleGUI::PreferredTextAreaMaxChars);
            if (SimpleGUI::Button(U"Load Directory", Vec2{ 1500, 75 }, 200)) {
                dir_loaded = true;
            }
        }
    }


    // 画像ファイルからテクスチャを作成する | Create a texture from an image file
    const Texture texture{ U"example/windmill.png" };
}
