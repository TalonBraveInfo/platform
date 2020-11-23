/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "plugin.h"

/*  Valve's VTF Format (https://developer.valvesoftware.com/wiki/Valve_Texture_Format)  */

PL_PACKED_STRUCT_START( VTFHeader )
unsigned int version[ 2 ];   // Minor followed by major.
unsigned int header_size;    // I guess this is used to support header alterations?
unsigned short width, height;// Width and height of the texture.
unsigned int flags;
unsigned short frames;    // For animated texture sets.
unsigned short firstframe;// Initial frame to start from.
unsigned char padding0[ 4 ];
float reflectivity[ 3 ];
unsigned char padding1[ 4 ];
float bumpmapscale;
unsigned int highresimageformat;
unsigned char mipmaps;
unsigned int lowresimageformat;
unsigned char lowresimagewidth;
unsigned char lowresimageheight;
PL_PACKED_STRUCT_END( VTFHeader )

typedef struct VTFHeader72 {
	unsigned short depth;
} VTFHeader72;

typedef struct VTFHeader73 {
	unsigned char padding2[ 3 ];
	unsigned int numresources;
} VTFHeader73;

#define VTF_VERSION_MAJOR 7
#define VTF_VERSION_MINOR 5

enum VTFFlag {
	VTF_FLAG_POINTSAMPLE = 0x00000001,
	VTF_FLAG_TRILINEAR = 0x00000002,
	VTF_FLAG_CLAMPS = 0x00000004,
	VTF_FLAG_CLAMPT = 0x00000008,
	VTF_FLAG_ANISOTROPIC = 0x00000010,
	VTF_FLAG_HINT_DXT5 = 0x00000020,
	VTF_FLAG_NO_COMPRESS = 0x00000040,
	VTF_FLAG_NORMAL = 0x00000080,
	VTF_FLAG_NOMIP = 0x00000100,
	VTF_FLAG_NOLOD = 0x00000200,
	VTF_FLAG_ALL_MIPS = 0x00000400,
	VTF_FLAG_PROCEDURAL = 0x00000800,
	VTF_FLAG_ONEBITALPHA = 0x00001000,  // Automatically generated by vtex from the texture data.
	VTF_FLAG_EIGHTBITALPHA = 0x00002000,// Automatically generated by vtex from the texture data.
	VTF_FLAG_ENVMAP = 0x00004000,
	VTF_FLAG_RENDERTARGET = 0x00008000,
	VTF_FLAG_DEPTHRENDERTARGET = 0x00010000,
	VTF_FLAG_NODEBUGOVERRIDE = 0x00020000,
	VTF_FLAG_SINGLECOPY = 0x00040000,
	VTF_FLAG_PRE_SRGB = 0x00080000,
	VTF_FLAG_PREMULTIPLY = 0x00100000,
	VTF_FLAG_DUDV = 0x00200000,
	VTF_FLAG_ALPHATESTMIPMAP = 0x00400000,
	VTF_FLAG_NODEPTHBUFFER = 0x00800000,
	VTF_FLAG_UNUSED_01000000 = 0x01000000,
	VTF_FLAG_CLAMPU = 0x02000000,
	VTF_FLAG_VERTEXTEXTURE = 0x04000000,
	VTF_FLAG_SSBUMP = 0x08000000,
	VTF_FLAG_UNUSED_10000000 = 0x10000000,
	VTF_FLAG_BORDER = 0x20000000,
} VTFFlag;

enum VTFFace {
	VTF_FACE_RIGHT,
	VTF_FACE_LEFT,
	VTF_FACE_BACK,
	VTF_FACE_FRONT,
	VTF_FACE_UP,
	VTF_FACE_DOWN,

	VTF_FACE_SPHEREMAP
} VTFFace;

enum VTFFormat {
	VTF_FORMAT_RGBA8888,
	VTF_FORMAT_ABGR8888,
	VTF_FORMAT_RGB888,
	VTF_FORMAT_BGR888,
	VTF_FORMAT_RGB565,
	VTF_FORMAT_I8,
	VTF_FORMAT_IA88,
	VTF_FORMAT_P8,
	VTF_FORMAT_A8,
	VTF_FORMAT_RGB888_BLUESCREEN,
	VTF_FORMAT_BGR888_BLUESCREEN,
	VTF_FORMAT_ARGB8888,
	VTF_FORMAT_BGRA8888,
	VTF_FORMAT_DXT1,
	VTF_FORMAT_DXT3,
	VTF_FORMAT_DXT5,
	VTF_FORMAT_BGRX8888,
	VTF_FORMAT_BGR565,
	VTF_FORMAT_BGRX5551,
	VTF_FORMAT_BGRA4444,
	VTF_FORMAT_DXT1_ONEBITALPHA,
	VTF_FORMAT_BGRA5551,
	VTF_FORMAT_UV88,
	VTF_FORMAT_UVWQ8888,
	VTF_FORMAT_RGBA16161616F,
	VTF_FORMAT_RGBA16161616,
	VTF_FORMAT_UVLX8888
} VTFFormat;

static void ConvertVTFFormat( PLImage *image, unsigned int in ) {
	switch ( in ) {
		case VTF_FORMAT_A8:
			image->format = PL_IMAGEFORMAT_RGB4;
			image->colour_format = PL_COLOURFORMAT_RGB;
			break;
		case VTF_FORMAT_ABGR8888:
			image->format = PL_IMAGEFORMAT_RGBA8;
			image->colour_format = PL_COLOURFORMAT_ABGR;
			break;
		case VTF_FORMAT_ARGB8888:
			image->format = PL_IMAGEFORMAT_RGBA8;
			image->colour_format = PL_COLOURFORMAT_ARGB;
			break;
		case VTF_FORMAT_BGR565:
			image->format = PL_IMAGEFORMAT_RGB565;
			image->colour_format = PL_COLOURFORMAT_BGR;
			break;
		case VTF_FORMAT_BGR888:
		case VTF_FORMAT_BGR888_BLUESCREEN:
			image->format = PL_IMAGEFORMAT_RGB8;
			image->colour_format = PL_COLOURFORMAT_BGR;
			break;
		case VTF_FORMAT_BGRA4444:
			image->format = PL_IMAGEFORMAT_RGBA4;
			image->colour_format = PL_COLOURFORMAT_BGRA;
			break;
		case VTF_FORMAT_BGRA5551:
			image->format = PL_IMAGEFORMAT_RGB5A1;
			image->colour_format = PL_COLOURFORMAT_BGRA;
			break;
		case VTF_FORMAT_BGRA8888:
		case VTF_FORMAT_BGRX8888:
			image->format = PL_IMAGEFORMAT_RGBA8;
			image->colour_format = PL_COLOURFORMAT_BGRA;
			break;
		case VTF_FORMAT_DXT1:
			image->format = PL_IMAGEFORMAT_RGB_DXT1;
			image->colour_format = PL_COLOURFORMAT_RGB;
			break;
		case VTF_FORMAT_DXT1_ONEBITALPHA:
			image->format = PL_IMAGEFORMAT_RGBA_DXT1;
			image->colour_format = PL_COLOURFORMAT_RGBA;
			break;
		case VTF_FORMAT_DXT3:
			image->format = PL_IMAGEFORMAT_RGBA_DXT3;
			image->colour_format = PL_COLOURFORMAT_RGBA;
			break;
		case VTF_FORMAT_DXT5:
			image->format = PL_IMAGEFORMAT_RGBA_DXT5;
			image->colour_format = PL_COLOURFORMAT_RGBA;
			break;
		case VTF_FORMAT_I8:
			abort();// todo
		case VTF_FORMAT_IA88:
			abort();// todo
		case VTF_FORMAT_P8:
			abort();// todo
		case VTF_FORMAT_RGB565:
			abort();           // todo
		case VTF_FORMAT_RGB888:// Same as RGB888_BLUESCREEN.
		case VTF_FORMAT_RGB888_BLUESCREEN:
			image->format = PL_IMAGEFORMAT_RGB8;
			image->colour_format = PL_COLOURFORMAT_RGB;
			break;
		case VTF_FORMAT_RGBA8888:
			image->format = PL_IMAGEFORMAT_RGBA8;
			image->colour_format = PL_COLOURFORMAT_RGBA;
			break;
		case VTF_FORMAT_RGBA16161616:
			image->format = PL_IMAGEFORMAT_RGBA16;
			image->colour_format = PL_COLOURFORMAT_RGBA;
		case VTF_FORMAT_RGBA16161616F:
			image->format = PL_IMAGEFORMAT_RGBA16F;
			image->colour_format = PL_COLOURFORMAT_RGBA;
			break;
		case VTF_FORMAT_UV88:
			abort();// todo
		case VTF_FORMAT_UVLX8888:
			abort();// todo
		case VTF_FORMAT_UVWQ8888:
			abort();// todo
		default:
			image->format = PL_IMAGEFORMAT_UNKNOWN;
			image->colour_format = PL_COLOURFORMAT_RGB;
	}
}

static bool VTF_ValidateFile( PLFile *file, VTFHeader *out ) {
	char magic[ 4 ];
	if ( gInterface->ReadFile( file, magic, sizeof( char ), 4 ) != 4 ) {
		return false;
	}

	if( strncmp( magic, "VTF", 3 ) != 0 ) {
		gInterface->ReportError( PL_RESULT_FILETYPE, "expected vtf, got %s", magic );
		return false;
	}

	if ( gInterface->ReadFile( file, out, sizeof( VTFHeader ), 1 ) != 1 ) {
		return false;
	}
	
#define VTF_VERSION( maj, min ) ( ( ( ( maj ) ) == out->version[ 1 ] && ( min ) <= out->version[ 0 ] ) || ( maj ) < out->version[ 0 ] )
	if ( VTF_VERSION( 7, 5 ) ) {
		gInterface->ReportError( PL_RESULT_FILEVERSION, "invalid version: %d.%d", out->version[ 1 ], out->version[ 0 ] );
		return false;
	}

	if ( !( plIsPowerOfTwo( out->width ) && plIsPowerOfTwo( out->height ) ) ) {
		gInterface->ReportError( PL_RESULT_IMAGERESOLUTION, "invalid resolution: %dx%d", out->width, out->height );
		return false;
	}

	if ( out->lowresimageformat != VTF_FORMAT_DXT1 ) {
		gInterface->ReportError( PL_RESULT_IMAGEFORMAT, "invalid texture format for lowresimage in VTF" );
		return false;
	}

	if ( ( out->lowresimagewidth > 16 ) || ( out->lowresimageheight > 16 ) ||
			( out->lowresimagewidth > out->width ) || ( out->lowresimageheight > out->height ) ) {
		gInterface->ReportError( PL_RESULT_IMAGERESOLUTION, "invalid resolution: %dx%d", out->width, out->height );
		return false;
	}

	return true;
}

PLImage *VTF_LoadImage( const char *path ) {
	PLFile *file = gInterface->OpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}
	
	VTFHeader header;
	if( !VTF_ValidateFile( file, &header ) ) {
		gInterface->CloseFile( file );
		return NULL;
	}

	// todo, use the headersize flag so we can load this more intelligently!

	VTFHeader72 header2;
	if ( header.version[ 1 ] >= 2 ) {
		if ( gInterface->ReadFile( file, &header2, sizeof( VTFHeader72 ), 1 ) != 1 ) {
			gInterface->CloseFile( file );
			return NULL;
		}
	}
	VTFHeader73 header3;
	if ( header.version[ 1 ] >= 3 ) {
		if ( gInterface->ReadFile( file, &header3, sizeof( VTFHeader73 ), 1 ) != 1 ) {
			gInterface->CloseFile( file );
			return NULL;
		}
	}
	
	PLImage *out = NULL;

	/*
    if (header.version[1] >= 3) {
        for (unsigned int i = 0; i < header3.numresources; i++) {
            // todo, support for later VTF versions.
        }
    } else */
	{
		unsigned int faces = 1;
		if ( header.flags & VTF_FLAG_ENVMAP ) {
			faces = 6;
		}

		// VTF's typically include a tiny thumbnail image at the start, which we'll skip.
		if ( !gInterface->FileSeek( file, header.lowresimagewidth * header.lowresimageheight / 2, PL_SEEK_CUR ) ) {
			return NULL;
		}

		memset( out, 0, sizeof( PLImage ) );

		out->width = header.width;
		out->height = header.height;

		ConvertVTFFormat( out, header.highresimageformat );

		out->levels = 1;
		out->data = ( uint8_t ** ) gInterface->CAlloc( 1, sizeof( uint8_t * ) );

		for ( int mipmap = 0; mipmap < header.mipmaps; ++mipmap ) {
			for ( unsigned int frame = 0; frame < header.frames; ++frame ) {
				for ( unsigned int face = 0, mipw = 1, miph = 1; face < faces; ++face ) {
					// We'll just skip the smaller mipmaps for now, can generate these later.
					mipw *= ( unsigned int ) pow( 2, mipmap );//(out->width * (mipmap + 1)) / header.mipmaps;
					miph *= ( unsigned int ) pow( 2, mipmap );//(out->height * (mipmap + 1)) / header.mipmaps;
					unsigned int mipsize = gInterface->GetImageSize( out->format, mipw, miph );
					if ( mipmap == ( header.mipmaps - 1 ) ) {
						out->data[ 0 ] = ( uint8_t * ) gInterface->CAlloc( mipsize, sizeof( uint8_t ) );
						if ( gInterface->ReadFile( file, out->data[ 0 ], sizeof( uint8_t ), mipsize ) != mipsize ) {
							gInterface->DestroyImage( out );
							return NULL;
						}
					} else {
						if ( !gInterface->FileSeek( file, ( long ) mipsize, SEEK_CUR ) ) {
							gInterface->DestroyImage( out );
							return NULL;
						}
					}

					if ( gInterface->IsEndOfFile( file ) ) {
						perror( PL_FUNCTION );
						break;
					}
				}
			}
		}
	}

	return out;
}