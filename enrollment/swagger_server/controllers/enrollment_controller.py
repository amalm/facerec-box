import connexion
import six
import csv
import os, fnmatch
import numpy as np
from PIL import Image
from io import BytesIO
import base64

from swagger_server.models.enrollment import Enrollment  # noqa: E501
from swagger_server import util
from swagger_server.face_vector.face_vector import FaceVector

faceVector = FaceVector()
storageFile = "users.csv"
storageFolder = "/home/inno/enrollment"


def fileParser(folder, filename):
    registrations = dict()
    if os.path.isfile(folder + '/' + filename):
        with open(folder + '/' + filename) as csv_file:
            csv_reader = csv.reader(csv_file, delimiter=' ')
            for row in csv_reader:
                if len(row) == 129:
                    array = np.zeros(128)
                    for i in range(0, 128):
                        array[i] = row[i+1]
                    registrations[row[0]] = array
    return registrations


def fileWriter(folder, filename, registrations):
    with open(folder + '/' + filename, mode='w+', newline='') as csv_file:
        registration_writer = csv.writer(csv_file, delimiter=' ', quotechar='"', quoting=csv.QUOTE_MINIMAL)
        
        for key, value in registrations.items():
            row_list = []
            row_list.append(key)
            for i in range(0, 128):
                row_list.append(value[i])
            registration_writer.writerow(row_list)


def find(pattern, path):
    result = []
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                result.append(os.path.join(root, name))
    return result


def delete_enrollment(name):  # noqa: E501
    """delete the enrollment for the ticket id

     # noqa: E501

    :param name: name (of person) to delete enrollment for
    :type name: str

    :rtype: None
    """
    return 'do some magic!'


def get_enrollment(name):  # noqa: E501
    """get the information of an enrollment

     # noqa: E501

    :param name: name (of persone) for which enrollment was done
    :type name: str

    :rtype: Enrollment
    """
    filenames = find(name + '.*', storageFolder)
    if len(filenames) > 0:
        file = open(filenames[0])
        return file, 200
    return 'No enrollment found!', 400


def get_enrollment_image(name, image_nr):  # noqa: E501
    """get an image enrolled for the name, enumeration starts w/ 0

     # noqa: E501

    :param name: name(of person) for which enrollment was done
    :type name: str
    :param image_nr: image number to get
    :type image_nr: int

    :rtype: ByteArray
    """
    return 'do some magic!'


def update_enrollment(name, file):  # noqa: E501
    """enrolls an identification with an image of the persons face

    If there is a enrolled identification the image is added to the set of images to be used as references. To remove a single reference image, use DELETE /enroll and then POST /enroll. # noqa: E501

    :param name: name of the holder of the ticket
    :type name: str
    :param file: file to upload
    :type file: werkzeug.datastructures.FileStorage

    :rtype: Enrollment
    """

    # check if storage folder exists
    if not os.path.exists(storageFolder):
        os.makedirs(storageFolder)
    
    extension = file.filename.split(".")[-1]

    # calculate the face vector
    img = Image.open(file)
    face_vec = faceVector.get_face_vector(img, storageFolder + '/' + name + '.' + extension)
    if face_vec is None:
        return 'No face detected!', 400
    
    # get all existing entries
    registration_dict = fileParser(storageFolder, storageFile)

    # check if the vector is similar to an existing vector and remove the old entry in this case
    twin = None
    for key, values in registration_dict.items():
        dist = np.linalg.norm(values - face_vec)
        if dist < 0.6:
            twin = key
            break
    
    if twin:
        # delete the images of the twin
        filenames = find(twin + '.*', storageFolder)
        for filename in filenames:
            os.remove(filename)
        # remove the entry in te dictionary
        del registration_dict[twin]

    # adding the new vector to the list
    registration_dict[name] = face_vec
    
    fileWriter(storageFolder, storageFile, registration_dict)
    #return "Success", 200
    enrollment = Enrollment(None, name, 1, None)
    return enrollment, 200


def update_enrollment_base64(name, image):  # noqa: E501
    """enrolls an identification with a Base64 image of the persons face

    If there is a enrolled identification the image is added to the set of images to be used as references. To remove a single reference image, use DELETE /enroll and then POST /enroll. # noqa: E501

    :param name: name of the holder of the ticket
    :type name: str
    :param image: Base64 image to upload
    :type image: str

    :rtype: Enrollment
    """

    img = Image.open(BytesIO(base64.b64decode(image.encode("utf-8"))))
    face_vec = faceVector.get_face_vector(img, storageFolder + '/' + name + '.jpg')
    if face_vec is None:
        return 'No face detected!', 400
    
    # get all existing entries
    registration_dict = fileParser(storageFolder, storageFile)

    # check if the vector is similar to an existing vector and remove the old entry in this case
    twin = None
    for key, values in registration_dict.items():
        dist = np.linalg.norm(values - face_vec)
        if dist < 0.6:
            twin = key
            break
    
    if twin:
        # delete the images of the twin
        filenames = find(twin + '.*', storageFolder)
        for filename in filenames:
            os.remove(filename)
        # remove the entry in te dictionary
        del registration_dict[twin]

    # adding the new vector to the list
    registration_dict[name] = face_vec
    
    fileWriter(storageFolder, storageFile, registration_dict)
    #return 'Success', 200
    enrollment = Enrollment(None, name, 1, None)
    return enrollment, 200
