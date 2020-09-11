#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <stdexcept>

#include <glm/glm.hpp>

#include "PPU466.hpp"


constexpr char USAGE_PROMPT[] = R"(
usage:
  ./asset_pipe_converter <input-tile-dir> <output-chunk-dir> <output-header-dir>

example:
  ./build_tools_bin/asset_pipe_converter assets/tiles/ dist/assets/ generated/include/
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
	int width;
	int height;
	std::vector<glm::u8vec4> data;
};

/**
 * load raw sprite PNG images from disk. throws exception on error
 *
 * @param tile_dir the directory that contains all the *.png sprites
 * @return the dict of ( key: resource name, value: PNG content )
 * @throw AssetConversionException if a failure happens
 */
std::map<std::string, ImgContent> load_raw_sprite_images(const std::string &tile_dir);

struct ProcessedSprites {
	std::vector<PPU466::Tile> tiles;
	std::vector<PPU466::Palette> palettes;
	// mapping: ( key: resource name, value: (tile_index, palette_index))
	std::map<std::string, std::pair<int,int>> mapping;
};
/**
 * read raw sprite images and attempt to convert to ProcessedSprites, which contains
 * PPU466-targeted tiles, palettes.
 *
 * @param raw_images the raw sprite images, returned by load_raw_sprite_images()
 * @return a ProcessedSprites struct that represents the tiles palettes and mapping
 * @throw AssetConversionException if a failure happens, e.g. when too many colors are used.
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
 * @throw process_sprite_images if any errors happened.
 */

void store_sprite_resources(const ProcessedSprites &sprites,
							const std::string &output_chunk_dir,
							const std::string &output_header_dir);

int main(int argc, char *argv[]) {
	if (argc != 3) {
		std::cout << USAGE_PROMPT << std::endl;
		return 1;
	}
	// TODO: implement me
	return 0;
}

std::map<std::string, ImgContent> load_raw_sprite_images(const std::string &tile_dir) {
	// TODO: implement me
	return std::map<std::string, ImgContent>();
}

ProcessedSprites process_sprite_images(const std::map<std::string, ImgContent> &raw_images) {
	// TODO: implement me
	return ProcessedSprites();
}

void store_sprite_resources(const ProcessedSprites &sprites,
							const std::string &output_chunk_dir,
							const std::string &output_header_dir) {
	// TODO: implement me
}
