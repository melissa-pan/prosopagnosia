

from PIL import Image
def rotate_image():
    # Create an Image object from an Image
    colorImage  = Image.open("./glasses.jpeg")
    # Rotate it by 90 degrees
    transposed  = colorImage.transpose(Image.ROTATE_90)
    # Display the Original Image
    colorImage.show() 
    # Display the Image rotated by 90 degrees
    transposed.show()