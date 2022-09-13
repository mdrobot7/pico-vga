import sys
import os
import cv2 as cv
import numpy as np

IMG_MAX_WIDTH = 320 #constants based on the Pico program
IMG_MAX_HEIGHT = 240

if len(sys.argv) < 2:
    print("Please specify a path to an image to convert.")
    exit()
elif not os.path.exists(sys.argv[1]):
    print("Image path does not exist. Please try again.")
    exit()

image = cv.imread(sys.argv[1])

#image = cv.cvtColor(image, cv.COLOR_BGR2RGB) #convert to RGB color space, important later

cv.imshow(sys.argv[1], image)
cv.waitKey(1)

while True:
    size = input(f"This image is currently {image.shape}.\n What would you like it to be resized to? [***x***]")
    size = size.split("x")
    size[0] = int(size[0])
    size[1] = int(size[1])

    if size[0] > IMG_MAX_WIDTH: print(f"The image must be less than {IMG_MAX_WIDTH}px wide.")
    elif size[1] > IMG_MAX_HEIGHT: print(f"The image must be less than {IMG_MAX_HEIGHT}px tall.")
    else:
        resized = cv.resize(image.copy(), size, interpolation = cv.INTER_AREA)
        cv.imshow("Resized image", resized)
        print("Resized. Press any key to retry with different dimensions, 'c' to continue.")
        if cv.waitKey(0) == ord("c"): break
        cv.destroyAllWindows()
        cv.imshow(sys.argv[1], image)
        cv.waitKey(1)

cv.destroyAllWindows()

binArray = np.zeros((size[1], size[0]), dtype = np.uint8) #make empty array, to be filled with 8 bit binary color

for i, x in enumerate(resized):
    for j, y in enumerate(x):
        y = y.tolist() #y is now the 3 bytes of RGB color data, in RGB order
        r = int(y[0]/32) #scale 0-255 down to 0-7, truncate
        g = int(y[1]/32)
        b = int(y[2]/64) #scale 0-255 down to 0-3, truncate

        r = r << 5 #shift the bits into their correct places
        g = g << 2
        b = b << 0

        binArray[i][j] = (r | g) | b #bitwise OR the 3 bin numbers, saves in Python in decimal so no conversion

filename = sys.argv[1][:sys.argv[1].rfind(".")] + ".c"

outputFile = open(filename, 'w')

outputFile.write(f"const uint8_t {filename[:-2]} [{size[1]}][{size[0]}] = " + "{\n")
for i, x in enumerate(binArray):
    outputFile.write(" {")
    for y in x[:-1]: outputFile.write(str(y) + (3 - len(str(y)))*" " + ", ")
    if i != len(binArray) - 1: outputFile.write(str(x[-1]) + "},\n")
    else: outputFile.write(str(x[-1]) + "} };")

outputFile.close()