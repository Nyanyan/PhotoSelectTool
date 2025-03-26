# include <Siv3D.hpp> // Siv3D v0.6.15
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <future>

uint16_t read16(const std::vector<uint8_t>& data, size_t offset, bool little_endian) {
    if (little_endian) {
        return data[offset] | (data[offset + 1] << 8);
    } else {
        return (data[offset] << 8) | data[offset + 1];
    }
}

uint32_t read32(const std::vector<uint8_t>& data, size_t offset, bool little_endian) {
    if (little_endian) {
        return data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24);
    } else {
        return (data[offset] << 24) | (data[offset + 1] << 16) | (data[offset + 2] << 8) | data[offset + 3];
    }
}

int getOrientation(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return -1;
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (data.size() < 2 || data[0] != 0xFF || data[1] != 0xD8) {
        std::cerr << "Not a valid JPEG file: " << file_path << std::endl;
        return -1;
    }

    size_t offset = 2;
    while (offset < data.size()) {
        if (data[offset] != 0xFF) {
            std::cerr << "Invalid marker at offset " << offset << std::endl;
            return -1;
        }

        uint8_t marker = data[offset + 1];
        uint16_t length = read16(data, offset + 2, false);

        if (marker == 0xE1) { // APP1 marker (Exif)
            size_t exif_offset = offset + 4;
            // if (data.size() < exif_offset + 6 || std::string(data.begin() + exif_offset, data.begin() + exif_offset + 6) != "Exif\0\0") {
            //     std::cerr << "Invalid Exif header" << std::endl;
            //     std::cerr << data.size() << " " << exif_offset << std::endl;
            //     return -1;
            // }

            size_t tiff_offset = exif_offset + 6;
            bool is_little_endian = read16(data, tiff_offset, false) == 0x4949;
            size_t ifd_offset = tiff_offset + read32(data, tiff_offset + 4, is_little_endian);

            size_t num_entries = read16(data, ifd_offset, is_little_endian);
            for (size_t i = 0; i < num_entries; ++i) {
                size_t entry_offset = ifd_offset + 2 + i * 12;
                uint16_t tag = read16(data, entry_offset, is_little_endian);
                if (tag == 0x0112) { // Orientation tag
                    uint16_t orientation = read16(data, entry_offset + 8, is_little_endian);
                    return orientation;
                }
            }
        }

        offset += 2 + length;
    }

    std::cerr << "Orientation tag not found" << std::endl;
    return -1;
}

struct JPG_Info {
    Texture texture;
    int orientation;
    bool selected;
};

JPG_Info open_jpg(std::string filename) {
    const Texture texture{Unicode::Widen(filename)};
    int orientation = getOrientation(filename);
    return {texture, orientation, false};
}

void Main() {
    constexpr int IMG_WIDTH = 6000;
    constexpr int IMG_HEIGHT = 4000;
    constexpr double const_scale = 0.3;
    Scene::SetBackground(Palette::Black);
    // Size window_size = Size((int)(IMG_WIDTH * scale), (int)(IMG_HEIGHT * scale));
    Size window_size = Size((int)(IMG_WIDTH * const_scale), (int)(IMG_HEIGHT * const_scale));
    Window::Resize(window_size);
    Window::SetTitle(U"PhotoSelectTool");
    Window::SetStyle(WindowStyle::Sizable);
    Scene::SetResizeMode(ResizeMode::Virtual);
    const Font font{ FontMethod::MSDF, 48, Typeface::Bold };

    Console.open();

    TextAreaEditState text_area[2];

    bool dir_loaded = false;

    std::string in_dir, out_dir;
    std::vector<std::string> jpg_files;
    std::vector<Texture> textures;
    std::vector<int> orientations;
    std::vector<bool> selected;

    int file_idx = 0;

    std::vector<std::future<int>> raw_futures;
    std::future<JPG_Info> jpg_future;

    while (System::Update()) {

        if (dir_loaded) {
            if (jpg_future.valid()) {
                if (jpg_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    JPG_Info jpg_info = jpg_future.get();
                    textures.emplace_back(jpg_info.texture);
                    orientations.emplace_back(jpg_info.orientation);
                    selected.emplace_back(jpg_info.selected);
                }
            } else {
                int load_file_idx = textures.size();
                if (load_file_idx < jpg_files.size()) {
                    jpg_future = std::async(std::launch::async, open_jpg, jpg_files[load_file_idx]);
                }
            }
            if (textures.size() > file_idx) {
                double size_x = Window::GetState().virtualSize.x;
                double size_y = Window::GetState().virtualSize.y;
                double scale_w = std::min((double)size_x / IMG_WIDTH, (double)size_y / IMG_HEIGHT);
                double scale_v = std::min((double)size_x / IMG_HEIGHT, (double)size_y / IMG_WIDTH);
                int x_c = size_x / 2;
                int y_c = size_y / 2;
                if (orientations[file_idx] == 6) {
                    textures[file_idx].scaled(scale_v).rotated(90_deg).drawAt(x_c, y_c);
                } else if (orientations[file_idx] == 3) {
                    textures[file_idx].scaled(scale_w).rotated(180_deg).drawAt(x_c, y_c);
                } else if (orientations[file_idx] == 8) {
                    textures[file_idx].scaled(scale_v).rotated(270_deg).drawAt(x_c, y_c);
                } else {
                    textures[file_idx].scaled(scale_w).drawAt(x_c, y_c);
                }
                if (selected[file_idx]) {
                    font(U"Selected").draw(30, Arg::topLeft(0, 0), Palette::Red);
                }
                Window::SetTitle(Unicode::Widen(jpg_files[file_idx]));
                for (std::future<int>& raw_future : raw_futures) {
                    if (raw_future.valid() && raw_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                        raw_future.get();
                    }
                }
                if (KeyEnter.down()) {
                    std::string raw_file = jpg_files[file_idx].substr(0, jpg_files[file_idx].size() - 4) + ".NEF";
                    std::cout << "copy " << jpg_files[file_idx] << " " << raw_file << std::endl;
                    try {
                        //std::filesystem::copy(jpg_files[file_idx], out_dir, std::filesystem::copy_options::overwrite_existing);
                        std::filesystem::copy(raw_file, out_dir, std::filesystem::copy_options::overwrite_existing);
                    } catch (const std::filesystem::filesystem_error& e) {
                        std::cout << "Error copying file: " << e.what() << std::endl;
                    }
                    selected[file_idx] = true;
                }
                if (KeySpace.down() && selected[file_idx]) {
                    std::string filename = jpg_files[file_idx].substr(jpg_files[file_idx].find_last_of("\\") + 1);
                    filename = filename.substr(0, filename.size() - 4);
                    std::string raw_file_dst = out_dir + "/" + filename + ".NEF";
                    std::cout << "open " << raw_file_dst << std::endl;
                    raw_futures.emplace_back(std::async(std::launch::async, system, raw_file_dst.c_str()));
                }
                if (KeyRight.down()) {
                    file_idx = std::min(file_idx + 1, (int)jpg_files.size() - 1);
                }
                if (KeyLeft.down()) {
                    file_idx = std::max(file_idx - 1, 0);
                }
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
                std::cout << "Loaded " << jpg_files.size() << " files" << std::endl;
                dir_loaded = true;
            }
            font(U"<- / ->: move\nEnter: Copy Image\nSpace: Open Copied RAW file").draw(30, Arg::topLeft(100, 400), Palette::White);
        }
    }
}
