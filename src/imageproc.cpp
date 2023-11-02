
#include <util.h>
#include <filter.h>



int main()
{
    std::cout << "Image Processing!!!" << std::endl;

    NormalImage img;
    img.loadImage("../images/cat.png");


    std::cout << "Image width: " << img.getWidth();
    std::cout << "Image height: " << img.getHeight();
    
}
