# include <Siv3D.hpp> // Siv3D v0.6.15
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

void Main() {
    Scene::SetBackground(Palette::Black);
    double scale = 0.3;
    Size window_size = Size((int)(6000 * scale), (int)(4000 * scale));
    Window::Resize(window_size);
    Window::SetTitle(U"PhotoSelectTool");
    const Font font{ FontMethod::MSDF, 48, Typeface::Bold };

    TextAreaEditState text_area[2];

    bool dir_loaded = false;

    std::string in_dir, out_dir;
    std::vector<std::string> jpg_files;
    std::vector<Texture> textures;

    int file_idx = 0;

    while (System::Update()) {

        if (dir_loaded) {
            if (textures.size() <= file_idx) {
                const Texture texture{Unicode::Widen(jpg_files[file_idx])};
                textures.emplace_back(texture);
            }
            textures[file_idx].scaled(scale).draw(0, 0);
            Window::SetTitle(Unicode::Widen(jpg_files[file_idx]));
            if (KeyRight.down()) {
                file_idx = std::min(file_idx + 1, (int)jpg_files.size() - 1);
            }
            if (KeyLeft.down()) {
                file_idx = std::max(file_idx - 1, 0);
            }
        } else {
            font(U"Input Directory: ").draw(30, Arg::topRight(400, 50), Palette::White);
            SimpleGUI::TextArea(text_area[0], Vec2{400, 50}, SizeF{1000, 30}, SimpleGUI::PreferredTextAreaMaxChars);
            font(U"Output Directory: ").draw(30, Arg::topRight(400, 100), Palette::White);
            SimpleGUI::TextArea(text_area[1], Vec2{400, 100}, SizeF{1000, 30}, SimpleGUI::PreferredTextAreaMaxChars);
            if (SimpleGUI::Button(U"Load Directory", Vec2{ 1500, 75 }, 200)) {
                in_dir = text_area[0].text.narrow();
                out_dir = text_area[1].text.narrow();
                for (const auto& entry : std::filesystem::directory_iterator(in_dir)) {
                    if (entry.path().extension() == ".JPG") {
                        jpg_files.push_back(entry.path().string());
                    }
                }
                dir_loaded = true;
            }
        }
    }
}
