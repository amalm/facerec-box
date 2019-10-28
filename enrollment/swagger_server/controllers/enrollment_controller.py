import connexion
import six

from swagger_server.models.byte_array import ByteArray  # noqa: E501
from swagger_server.models.enrollment import Enrollment  # noqa: E501
from swagger_server import util


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
    return 'do some magic!'


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
    return 'do some magic!'
