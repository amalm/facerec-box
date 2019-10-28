import connexion
import six

from swagger_server.models.key_values import KeyValues  # noqa: E501
from swagger_server import util


def get_statistics():  # noqa: E501
    """returns a key/value map describing relevant statistics

     # noqa: E501


    :rtype: KeyValues
    """
    print("test")
    return 'do some magic!'


def get_version():  # noqa: E501
    """returns the version of the Face Recognition Box implementing this API

     # noqa: E501


    :rtype: str
    """
    return 'do some magic!'
