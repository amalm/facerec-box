# coding: utf-8

from __future__ import absolute_import

from flask import json
from six import BytesIO

from swagger_server.models.key_values import KeyValues  # noqa: E501
from swagger_server.test import BaseTestCase


class TestServerController(BaseTestCase):
    """ServerController integration test stubs"""

    def test_get_statistics(self):
        """Test case for get_statistics

        returns a key/value map describing relevant statistics
        """
        response = self.client.open(
            '//statistics',
            method='GET')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))

    def test_get_version(self):
        """Test case for get_version

        returns the version of the Face Recognition Box implementing this API
        """
        response = self.client.open(
            '//version',
            method='GET')
        self.assert200(response,
                       'Response body is : ' + response.data.decode('utf-8'))


if __name__ == '__main__':
    import unittest
    unittest.main()
