from __future__ import absolute_import
from __future__ import print_function
import PIL
import torch
import glob as gb
import numpy as np
from PIL import Image
from swagger_server.face_vector.detector import detect_faces


# hyper parameters
mean = (131.0912, 103.8827, 91.4953)


def load_data_img(img, shape=None):
    short_size = 224.0
    crop_size = shape
    im_shape = np.array(img.size)    # in the format of (width, height, *)
    img = img.convert('RGB')

    ratio = float(short_size) / np.min(im_shape)
    img = img.resize(size=(int(np.ceil(im_shape[0] * ratio)),   # width
                           int(np.ceil(im_shape[1] * ratio))),  # height
                     resample=PIL.Image.BILINEAR)

    x = np.array(img)  # image has been transposed into (height, width)
    newshape = x.shape[:2]
    h_start = (newshape[0] - crop_size[0])//2
    w_start = (newshape[1] - crop_size[1])//2
    x = x[h_start:h_start+crop_size[0], w_start:w_start+crop_size[1]]
    x = x - mean
    return x


def chunks(l, n):
    # For item i in a range that is a length of l,
    for i in range(0, len(l), n):
        # Create an index range for l of n items:
        yield l[i:i+n]


def initialize_model():
    # Download the pytorch model and weights.
    # Currently, it's cpu mode.
    #from ...models.resnet50_128_pytorch.resnet50_128 
    import resnet50_128 as model
    network = model.resnet50_128(weights_path='resnet50_128.pth')
    network.eval()
    return network


def image_encoding(model, img):
    print('==> compute image-level feature encoding.')

    im_array = np.array([load_data_img(img, shape=(224, 224, 3))])
    f = model(torch.Tensor(im_array.transpose(0, 3, 1, 2)))[1].detach().cpu().numpy()[:, :, 0, 0]
    return f[0]


def get_face_vector(filename, savefilename=''):
    # open image and get all faces within the image
    img = Image.open(filename)
    bounding_boxes, landmarks = detect_faces(img)

    # find the largest bounding box (i.e. the largest face)
    max_area = 0
    max_bounding_box = None
    for bounding_box in bounding_boxes:
        width = bounding_box[2] - bounding_box[0]
        height = bounding_box[3] - bounding_box[1]
        area = width * height
        if area > max_area:
            max_bounding_box = bounding_box
            max_area = area

    # in case no face has found return None
    if max_bounding_box is None:
        return None
    
    # crop the image to only the largest face
    img = img.crop((max_bounding_box[0], max_bounding_box[1], max_bounding_box[2], max_bounding_box[3]))
    if savefilename != '':
        img.save(savefilename)

    # calculate the face vector for the largest face within the image
    model_eval = initialize_model()
    face_vec = image_encoding(model_eval, img)
    return face_vec


if __name__ == '__main__':
    face_vec = get_face_vector('samples/multiple.jpg', 'test3.jpg')
    print(face_vec)