
#include <util.h>



template <>
unsigned char convertToGray(std::complex<double> pixel)
{
    return static_cast<unsigned char>(pixel.real());
}
