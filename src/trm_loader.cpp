#include <trm_loader.h>
#include <render_device.h>
#include <fstream>
#include <macros.h>

#define READ_AND_OFFSET(stream, dest, size, offset) stream.read((char*)dest, size); offset += size; stream.seekg(offset);

namespace trm
{
	struct ImageHeader
	{
		uint8_t  compression;
		uint8_t  channelSize;
		uint8_t  numChannels;
		uint16_t numArraySlices;
		uint8_t  numMipSlices;
	};

	struct MipSliceHeader
	{
		uint16_t width;
		uint16_t height;
		int size;
	};

	struct FileHeader
	{
		uint32_t magic;
		uint8_t  version;
		uint8_t  type;
	};

	Texture* load_image(std::string file, int format, RenderDevice* device)
	{
		Texture2DCreateDesc tex2d_desc;
		TextureCubeCreateDesc texcube_desc;
		DW_ZERO_MEMORY(tex2d_desc);
		DW_ZERO_MEMORY(texcube_desc);

		Texture* texture;

		std::fstream f(file, std::ios::in | std::ios::binary);

		FileHeader fileheader;
		uint16_t nameLength = 0;
		char name[256];
		ImageHeader imageHeader;

		long offset = 0;

		f.seekp(offset);

		READ_AND_OFFSET(f, &fileheader, sizeof(FileHeader), offset);
		READ_AND_OFFSET(f, &nameLength, sizeof(uint16_t), offset);
		READ_AND_OFFSET(f, &name[0], sizeof(char) * nameLength, offset);

		name[nameLength] = '\0';

#if defined(TRM_PRINT_DEBUG_INFO)
		std::cout << "Name: " << name << std::endl;
#endif

		READ_AND_OFFSET(f, &imageHeader, sizeof(ImageHeader), offset);

#if defined(TRM_PRINT_DEBUG_INFO)
		std::cout << "Channel Size: " << imageHeader.channelSize << std::endl;
		std::cout << "Channel Count: " << imageHeader.numChannels << std::endl;
		std::cout << "Array Slice Count: " << imageHeader.numArraySlices << std::endl;
		std::cout << "Mip Slice Count: " << imageHeader.numMipSlices << std::endl;
#endif

		if (imageHeader.numArraySlices == 6)
		{
			texcube_desc.mipmapLevels = imageHeader.numMipSlices;
			texcube_desc.format = format;
		}
		else
		{
			tex2d_desc.mipmap_levels = imageHeader.numMipSlices;
			tex2d_desc.format = format;
		}
		
		for (int arraySlice = 0; arraySlice < imageHeader.numArraySlices; arraySlice++)
		{
#if defined(TRM_PRINT_DEBUG_INFO)
			std::cout << std::endl;
			std::cout << "Array Slice: " << arraySlice << std::endl;
#endif

			for (int mipSlice = 0; mipSlice < imageHeader.numMipSlices; mipSlice++)
			{
				MipSliceHeader mipHeader;
				char* imageData;

				READ_AND_OFFSET(f, &mipHeader, sizeof(MipSliceHeader), offset);

				if (arraySlice == 0 && mipSlice == 0)
				{
					if (imageHeader.numArraySlices == 6)
					{
						texcube_desc.width = mipHeader.width;
						texcube_desc.height = mipHeader.height;
						texture = device->create_texture_cube(texcube_desc);
					}
					else
					{
						tex2d_desc.width = mipHeader.width;
						tex2d_desc.height = mipHeader.height;
						texture = device->create_texture_2d(tex2d_desc);
					}
				}

#if defined(TRM_PRINT_DEBUG_INFO)
				std::cout << std::endl;
				std::cout << "Mip Slice: " << mipSlice << std::endl;
				std::cout << "Width: " << mipHeader.width << std::endl;
				std::cout << "Height: " << mipHeader.height << std::endl;
#endif

				imageData = (char*)malloc(mipHeader.size);
				READ_AND_OFFSET(f, imageData, mipHeader.size, offset);

				int array_slice = TextureType::TEXTURE2D;

				if (imageHeader.numArraySlices == 6)
					array_slice = TextureType::TEXTURECUBE + arraySlice + 1;
					
				device->set_texture_data(texture,
										 mipSlice,
										 array_slice,
										 mipHeader.width,
										 mipHeader.height,
										 imageData);

				free(imageData);
			}
		}

		f.close();

		return texture;
	}
}