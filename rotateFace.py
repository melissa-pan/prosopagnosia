import os
from PIL import Image

directory = "./face_database/"
def rotate_image():
    for filename in os.listdir(directory):
        if filename.endswith(".jpg"):
            colorImage = Image.open(os.path.join(directory, filename))
            colorImage = colorImage.transpose(Image.ROTATE_90)
            colorImage = colorImage.transpose(Image.ROTATE_90)
            colorImage.save(os.path.join(directory, filename))
            continue
        else:
            continue

