
#ifndef UTIL_H
#define UTIL_H

#include <png.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <complex>



template <typename Pixel>
class Image
{
    public:
        // Default Constructor
        Image();

        // Copy Constructor
        Image(Image& img);
        
        // Default Destructor
        ~Image();

        // "=" operator overload
        Image& operator=(Image& img);

        // Pixel operations
        Pixel& getPixel(int x, int y);

        // Width and Height getters
        int getWidth();
        int getHeight();

        // Load and save images
        bool loadImage(std::string filename);
        bool saveImage(std::string filename);

        // Resizes image
        void resizeImage(int new_width, int new_height);


    private:
        Pixel* _data;
        int _width;
        int _height;

        // Converts coordinate (x, y) to array index which we use to index into _data
        int getIndexFromCoordinates(int x, int y);
};


// Typedefs for both integer and complex valued images
typedef Image<int> NormalImage;
typedef Image<std::complex<double>> ComplexImage;


// Default Constructor Implementation
template <typename Pixel>
Image<Pixel>::Image()
{
    _data = nullptr;
    _width = 0;
    _height = 0;
}


// Copy Constructor Implementation
template <typename Pixel>
Image<Pixel>::Image(Image& img)
{
    *this = img;
}


// Default Destructor Implementation
template <typename Pixel>
Image<Pixel>::~Image()
{
    resizeImage(0, 0);
}


template <typename Pixel>
Image<Pixel>& Image<Pixel>::operator=(Image<Pixel>& img)
{
    resizeImage(img.getWidth(), img.getHeight());

    for (int i=0; i<img.getWidth(); i++)
    {
        for (int j=0; j<img.getHeight(); j++)
        {
            this->getPixel(i, j) = img.getPixel(i, j);
        }
    }
    return *this;
}


template <typename Pixel>
Pixel& Image<Pixel>::getPixel(int x, int y)
{
    return this->_data[getIndexFromCoordinates(x, y)];
}


template <typename Pixel>
int Image<Pixel>::getWidth()
{
    return this->_width;
}


template <typename Pixel>
int Image<Pixel>::getHeight()
{
    return this->_height;
}


template <typename Pixel>
bool Image<Pixel>::loadImage(std::string filename)
{
    // Open the PNG file for reading
    FILE *fp = fopen(filename.c_str(), "r");
    if (!fp)
    {
        std::cout << "Could not open file!" << std::endl;
        return false;
    }

    // Read the first four bytes of the file to see if it is an PNG
    size_t num_bytes = 4;
    unsigned char* header;
    fread(header, 1, num_bytes, fp);
    bool is_png = !png_sig_cmp(header, 0, num_bytes);

    if (!is_png)
    {
        std::cout << "File is not a PNG!" << std::endl;
        return false;
    }

    // Allocate and initialize png_struct and png_info
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr)
    {
        fclose(fp);
        std::cout << "Could not create png_struct!" << std::endl;
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, (png_infopp) nullptr, (png_infopp) nullptr);
        std::cout << "Could not create png_info!" << std::endl;
        return false;
    }


    // Set error handling
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        fclose(fp);
        std::cout << "Jmp Error!" << std::endl;
        return false;
    }


    // Setup the input code
    png_init_io(png_ptr, fp);

    // Let libpng know that we read num_bytes from the file already
    png_set_sig_bytes(png_ptr, num_bytes);

    // Start reading the PNG file information up to the actual image data
    // Read (https://www.w3.org/TR/PNG-Chunks.html) for more info on PNG chunk specifications
    png_read_info(png_ptr, info_ptr);



    // Variables for the image header (IHDR)
    // Compression and filter variables are left out since only one method is currently defined for them
	png_uint_32 width;
    png_uint_32 height;
	int bit_depth;
    int color_type;
    int interlace_type;

    // Reads the IHDR chunk and stores the data in it into our variables
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, nullptr, nullptr);


    // Truncate 16 bits/pixel to 8 bits/pixel
	if (bit_depth == 16)
	{
		png_set_strip_16(png_ptr);
	}

	// Expand paletted colors into RGB color values
	if (color_type == PNG_COLOR_TYPE_PALETTE) 
	{
        png_set_palette_to_rgb(png_ptr);
	}

    // Expand grayscale images to full 8 bits/pixel
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
	{
        png_set_expand_gray_1_2_4_to_8(png_ptr);
	}

	// Expand paletted or RGB images with transparency to full alpha channels
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) 
	{
        png_set_tRNS_to_alpha(png_ptr);
	}

	// Update the palette and info structure
	png_read_update_info(png_ptr, info_ptr);


    // Start reading the actual image data
    std::vector<png_bytep> row_pointers(height);
	int num_chan = png_get_channels(png_ptr, info_ptr);
	int num_row_bytes = png_get_rowbytes(png_ptr, info_ptr);
	for (int row=0; row<height; row++) 
	{
        row_pointers[row] = (png_bytep) malloc(num_row_bytes);
	}
	png_read_image(png_ptr, &row_pointers.front());
	png_read_end(png_ptr, info_ptr);


    // Warn if the image has more channels than we can handle
	if (num_chan != 1) 
	{
		std::cout << "Warning: " << filename << " has more than one channel (only reading the first)." << std::endl;
	}

	// Read the data and convert it's pixel type
	resizeImage(width, height);
	for (int row=0; row<this->_height; row++) 
	{
		for (int column=0; column<this->_width; column++) 
		{
			getPixel(column, row) = Pixel(row_pointers[row][column*num_chan]);
		}
	}

	// Free the memory and close the file
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	for (int row=0; row<height; row++)
	{
		free(row_pointers[row]);
	}
    fclose(fp);
    return true;
}


template <typename Pixel>
bool Image<Pixel>::saveImage(std::string filename)
{
    // Open the PNG file for writing binary
    FILE *fp = fopen(filename.c_str(), "wb");
	if (!fp) 
	{
        std::cout << "Could not open file!" << std::endl;
		return false;
	}

    // Allocate and initialize png_struct and png_info
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr)
    {
        fclose(fp);
        std::cout << "Could not create png_struct!" << std::endl;
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, (png_infopp) nullptr);
        std::cout << "Could not create info_ptr!" << std::endl;
        return false;
    }

    // Set error handling
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, (png_infopp) nullptr);
        std::cout << "Jmp Error!" << std::endl;
        return false;
    }

    // Setup the output code
    png_init_io(png_ptr, fp);

    // Set the IHDR to write to the file
    int img_type = PNG_COLOR_TYPE_GRAY;
    png_set_IHDR(png_ptr, info_ptr, this->_width, this->_height, 8, img_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // Start writing the PNG file information up to the actual image data
    png_write_info(png_ptr, info_ptr);


    // Convert the pixel data to bytes so that we can pass it to the PNG writer
	unsigned char* pixel_data_as_byte = new unsigned char[this->_width * this->_height];
	for (int i=0; i<(this->_width*this->_height); i++) 
	{
		pixel_data_as_byte[i] = convertToGray(this->_data[i]);
	}

	std::vector<png_bytep> row_pointers(this->_height);
	for(int j=0; j<this->_height; j++) 
	{
		row_pointers[j] = (png_bytep) &pixel_data_as_byte[j*this->_width];
	}

    // Write the image data into the file
    png_write_image(png_ptr, &row_pointers.front());
    png_write_end(png_ptr, info_ptr);

    // Free memory and close the file
    png_destroy_write_struct(&png_ptr, (png_infopp) nullptr);
    fclose(fp);
    delete[] pixel_data_as_byte;
    return true;
}


template <typename Pixel>
void Image<Pixel>::resizeImage(int new_width, int new_height)
{
    // Wipe current Image
    this->_width = 0;
    this->_height = 0;

    delete[] this->_data;
    this->_data == nullptr;


    // Check if new_width and new_height make sense
    if (new_width < 0 || new_height < 0)
    {
        std::cout << "New image dimensions " << "(" << new_width << ", " << new_height << ")" << "are invalid!" << std::endl;
        throw "Invalid image dimensions";
    }


    // Generate new image
    this->_width = new_width;
    this->_height = new_height;

    int num_pixels = (this->_width) * (this->_height);
    this->_data = new Pixel[num_pixels];

    if (num_pixels != 0 && _data == nullptr)
    {
        std::cout << "Could not allocate memory for image!" << std::endl;
        throw "Could not allocate memory for image!";
    }
}


template <typename Pixel>
int Image<Pixel>::getIndexFromCoordinates(int x, int y)
{
    if (this->_width == 0 || this->_height == 0)
    {
        std::cout << "Image is empty!" << std::endl;
        throw "Image is empty!";
    }

    if (x < 0 || x >= this->_width || y < 0 || y >= this->_height)
    {
        std::cout << "Warning: The pixel coordinate (" << x << "," << y << ") is invalid, clamping coordinates to the image's dimensions" << std::endl;
    }

    x = std::min(x, this->_width - 1);
    x = std::max(x, 0);
    y = std::min(y, this->_height - 1);
    y = std::max(y, 0);

    return ((y*this->_width) + x);
}


// Converts pixel from "Pixel" type to an "unsigned char" for writing to files
template <typename Pixel>
unsigned char convertToGray(Pixel pixel)
{    
    return static_cast<unsigned char>(pixel);
}

// Converts pixel from "complex" type to an "unsigned char" for writing to files
template <>
unsigned char convertToGray(std::complex<double> pixel);



#endif // UTIL_H
