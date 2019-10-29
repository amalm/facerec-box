# coding: utf-8

from __future__ import absolute_import

from flask import json
from six import BytesIO

from swagger_server.models.byte_array import ByteArray  # noqa: E501
from swagger_server.models.enrollment import Enrollment  # noqa: E501
from swagger_server.test import BaseTestCase


class TestEnrollmentController(BaseTestCase):
    """EnrollmentController integration test stubs"""

    def test_delete_enrollment(self):
        """Test case for delete_enrollment

        delete the enrollment for the ticket id
        """
        response = self.client.open(
            '//enroll/{name}'.format(name='name_example'),
            method='DELETE')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_get_enrollment(self):
        """Test case for get_enrollment

        get the information of an enrollment
        """
        response = self.client.open(
            '//enroll/{name}'.format(name='name_example'),
            method='GET')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_get_enrollment_image(self):
        """Test case for get_enrollment_image

        get an image enrolled for the name, enumeration starts w/ 0
        """
        response = self.client.open(
            '//enroll/{name}/{image_nr}'.format(name='name_example', image_nr=56),
            method='GET')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_update_enrollment(self):
        """Test case for update_enrollment

        enrolls an identification with an image of the persons face
        """
        data = dict(name='name_example',
                    file=(BytesIO(b'some file data'), 'file.txt'))
        response = self.client.open(
            '//enroll',
            method='POST',
            data=data,
            content_type='multipart/form-data')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))


if __name__ == '__main__':
    import unittest
    unittest.main()
