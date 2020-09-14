#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <utility>
#include <algorithm>
#include <cassert>
#include <filesystem>

#include <glm/glm.hpp>

#include "PPU466.hpp"
#include "read_write_chunk.hpp"
#include "load_save_png.hpp"

namespace fs = std::filesystem;

constexpr char USAGE_PROMPT[] = R"(
usage:
  ./asset_pipe_converter <input-tile-dir> <output-chunk-dir> <output-header-dir>

example:
  ./build_tools_bin/asset_pipe_converter assets/sprites/ dist/assets/ generated/include/
)";

/**
 * AssetConversionException: represents an error during the asset pipeline.
 *
 * The errmsg in the constructor should be a user readable string explaining the error reason
 */
class AssetConversionException : public std::runtime_error {
public:
	explicit AssetConversionException(const std::string &errmsg) : runtime_error(errmsg) {}
	explicit AssetConversionException(const char *errmsg) : runtime_error(errmsg) {}
	~AssetConversionException() override = default;
};

struct ImgContent {
	glm::uvec2 size{};
	std::vector<glm::u8vec4> data;
};

/**
 * load raw sprite PNG images from disk. throws exception on error
 *
 * @param tile_dir the directory that contains all the *.png sprites
 * @return the dict of ( key: resource name, value: PNG content )
 * @throw exception if a failure happens
 */
std::map<std::string, ImgContent> load_raw_sprite_images(const std::string &tile_dir);

struct ProcessedSprites {
	std::vector<PPU466::Tile> tiles;
	std::vector<PPU466::Palette> palettes;
	// mapping: ( key: resource name, value: (tile_index, palette_index))
	std::map<std::string, std::pair<int, int>> mapping;
};
/**
 * read raw sprite images and attempt to convert to ProcessedSprites, which contains
 * PPU466-targeted tiles, palettes.
 *
 * @param raw_images the raw sprite images, returned by load_raw_sprite_images()
 * @return a ProcessedSprites struct that represents the tiles palettes and mapping
 * @throw exception if a failure happens, e.g. when too many colors are used.
 */
ProcessedSprites process_sprite_images(const std::map<std::string, ImgContent> &raw_images);

/**
 * Give the processed sprites, save it to disk. It includes:
 *   $output_chunk_dir/tiles.chunk,
 *   $output_chunk_dir/palettes.chunk,
 *   $output_header_dir/assets_res.h,
 *
 * @param sprites the processed sprites, from process_sprite_images() function
 * @param output_chunk_dir the destination for .chunk files,
 *   should be either relative to current working directory, or an absolute path.
 *   will be created automatically if it doesn't exist.
 * @param output_header_dir the destnation for generated c++ header files
 *   Should be either relative to current working directory, or an absolute path.
 *   Will be created automatically if it doesn't exist.
 * @throw exception if any errors happened.
 */

void store_sprite_resources(
	const ProcessedSprites &sprites,
	const std::string &output_chunk_dir,
	const std::string &output_header_dir);
void store_sprite_header_file(const ProcessedSprites &sprites, const std::string &output_header_dir);
void store_sprite_chunk_file(const ProcessedSprites &sprites, const std::string &output_chunk_dir);

int main(int argc, char *argv[]) {
	if (argc != 4) {
		std::cout << USAGE_PROMPT << std::endl;
		return 1;
	}
	// TODO: implement me
	try {
		std::map<std::string, ImgContent> raw_images = load_raw_sprite_images(argv[1]);
		ProcessedSprites processed_sprites = process_sprite_images(raw_images);
		store_sprite_resources(processed_sprites, argv[2], argv[3]);
		return 0;
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 1;
}

std::map<std::string, ImgContent> load_raw_sprite_images(const std::string &tile_dir) {
	std::map<std::string, ImgContent> mapping;
	for (const auto& entry : fs::directory_iterator(tile_dir)){
		const auto filename_str = entry.path().filename().string();
		std::string extension = filename_str.substr(filename_str.find('.')+1);
		if (extension == "png"){
			ImgContent img;
			load_png(entry.path(), &img.size, &img.data, LowerLeftOrigin);
			std::string name = filename_str.substr(0, filename_str.find('.'));
			mapping[name] = img;
		}
	}
	std::cout<<"loading sprite images of: ";
	for (auto const& [key, val] : mapping){
		std::cout << key << " ";
	}
	std::cout<<std::endl;
	return mapping;
}

ProcessedSprites process_sprite_images(const std::map<std::string, ImgContent> &raw_images) {
	std::vector<PPU466::Tile> tiles;
	std::vector<PPU466::Palette> palettes;
	std::map<std::string, std::pair<int,int>> mapping;
	for (const auto &img_iterator : raw_images) {
		const std::string &name = img_iterator.first;
		const ImgContent &img = img_iterator.second;
		int tile_index = -1;
		int palette_index = -1;
		if (img.size[0] != 8 || img.size[1] != 8) {
			throw AssetConversionException(
				std::string("Invalid PNG asset size for ") + name + ". Should be 8x8, but actually"
					+ std::to_string(img.size[0]) + "x" + std::to_string(img.size[1]));
		}
		std::vector<glm::u8vec4> colors;
		// first pass: what's the colors in this img
		for (const auto &pix : img.data) {
			auto it = std::find(colors.begin(), colors.end(), pix);
			if (it != colors.end()) {
				// no action required
			} else {
				colors.push_back(pix);
				if (colors.size() > 4) {
					std::string errmsg = std::string("Too many colors used in asset ") + name + ".";
					throw AssetConversionException(errmsg);
				}
			}
		}
		// sort colors in a canonical order: First order by alpha in ascending order,
		// on tie, order by red in ascending order, then by green, then by blue
		std::sort(colors.begin(), colors.end(),
				  [](const glm::u8vec4 &a, const glm::u8vec4 &b) -> bool {
					  if (a[3] != b[3]) {
						  return a[3] < b[3];
					  } else if (a[0] != b[0]) {
						  return a[0] < b[0];
					  } else if (a[1] != b[1]) {
						  return a[1] < b[1];
					  } else {
						  return a[2] < b[2];
					  }
				  });
		assert(colors.size() <= 4);
		// pad zeroes if colors.size() < 4;
		colors.resize(4, glm::u8vec4{0, 0, 0, 0});
		PPU466::Palette p;
		std::copy(colors.begin(), colors.end(), p.begin());
		const auto palette_it = std::find(palettes.begin(), palettes.end(), p);
		for (const auto &c : p) {
			printf("Colors used in %s: #%02hhx%02hhx%02hhx%02hhx\n", name.c_str(), c.x, c.y, c.z, c.w);
		}
		if (palette_it != palettes.end()) {
			palette_index = std::distance(palettes.begin(), palette_it);
		} else {
			palettes.push_back(p);
			palette_index = palettes.size() - 1;
			if (palettes.size() > 8) {
				throw AssetConversionException("Too many palettes, exceeds 8");
			}
		}
		printf("palette idx for %s: %d\n", name.c_str(), palette_index);
		// second pass: convert the png content to tiles
		// TODO(xiaoqiao): is this zero initialization?
		PPU466::Tile t{};
		for (int i = 0; i < 8 * 8; i++) {
			int row_idx = i / 8;
			int col_idx = i % 8;
			const auto &pix = img.data[i];
			assert(std::find(p.begin(), p.end(), pix) != p.end());
			int pixel_color_idx_in_palette = std::distance(p.begin(), std::find(p.begin(), p.end(), pix));
			assert(pixel_color_idx_in_palette < 4);
			uint8_t bit0 = pixel_color_idx_in_palette & 1;
			uint8_t bit1 = pixel_color_idx_in_palette >> 1;
			t.bit0[row_idx] |= (bit0 << col_idx);
			t.bit1[row_idx] |= (bit1 << col_idx);
 		}
		tiles.push_back(t);
		tile_index = tiles.size() - 1;
		printf("tile idx for %s: %d\n", name.c_str(), tile_index);
		if (tiles.size() > 16 * 16) {
			throw AssetConversionException("Too many tiles: exceeds 16*16");
		}
		mapping[name] = std::make_pair(tile_index, palette_index);
	}
	return ProcessedSprites{std::move(tiles), std::move(palettes), std::move(mapping)};
}

void store_sprite_resources(
	const ProcessedSprites &sprites,
	const std::string &output_chunk_dir,
	const std::string &output_header_dir) {
	store_sprite_header_file(sprites, output_header_dir);
	store_sprite_chunk_file(sprites, output_chunk_dir);
}

void store_sprite_header_file(const ProcessedSprites &sprites, const std::string &output_header_dir) {
	fs::create_directories(output_header_dir);
	std::ofstream header_file_stream(output_header_dir + "/assets_res.h", std::ios::binary);
	if (!header_file_stream) {
		throw AssetConversionException("Error opening assets_res.h");
	}
	header_file_stream << "#pragma once\n";
	for (const auto &m : sprites.mapping) {
		// write f"#define ${uppercase(resource_name)}_TILE_IDX ${tile_idx}\n"
		header_file_stream << "#define ";
		for (const char c : m.first) { header_file_stream << (char) toupper(c); }
		header_file_stream << "_TILE_IDX " << m.second.first << "\n";
		// write f"#define ${uppercase(resource_name)}_PALETTE_IDX ${palette_idx}\n"
		header_file_stream << "#define ";
		for (const char c : m.first) { header_file_stream << (char) toupper(c); }
		header_file_stream << "_PALETTE_IDX " << m.second.second << "\n";
	}
	if (header_file_stream.fail()) {
		throw AssetConversionException("Error writing to assets_res.h");
	}
	header_file_stream.close();
}

void store_sprite_chunk_file(const ProcessedSprites &sprites, const std::string &output_chunk_dir) {
	fs::create_directories(output_chunk_dir);
	std::ofstream tile_stream(output_chunk_dir + "/tiles.chunk", std::ios::binary);
	std::ofstream palette_stream(output_chunk_dir + "/palettes.chunk", std::ios::binary);
	if (!tile_stream || !palette_stream) {
		throw AssetConversionException("error opening chunk files");
	}
	write_chunk("til0", sprites.tiles, &tile_stream);
	write_chunk("plt0", sprites.palettes, &palette_stream);
	if (tile_stream.fail() || palette_stream.fail()) {
		throw AssetConversionException("error writing chunk files");
	}
}

