#!/usr/bin/env python
# Copyright (C) 2015-2021, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import os
import sys
from unittest.mock import patch, MagicMock

import pytest

with patch('wazuh.common.ossec_uid'):
    with patch('wazuh.common.ossec_gid'):
        sys.modules['wazuh.rbac.orm'] = MagicMock()
        import wazuh.rbac.decorators

        from wazuh.tests.util import get_fake_database_data, RBAC_bypasser, InitWDBSocketMock

        wazuh.rbac.decorators.expose_resources = RBAC_bypasser
        from wazuh import mitre

test_data_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data')


# Fixtures
@pytest.fixture(scope='module')
def mitre_db():
    """Get fake MITRE database cursor."""
    return get_fake_database_data('schema_mitre_test.sql').cursor()


# Functions
class CursorByName:
    """Class to return query result including the column name as key."""
    def __init__(self, cursor):
        self._cursor = cursor

    def __iter__(self):
        return self

    def __next__(self):
        row = self._cursor.__next__()
        return {description[0]: row[col] for col, description in enumerate(self._cursor.description)}


def mitre_query(cursor, query):
    """Return list of dictionaries with the query results."""
    cursor.execute(query)
    return [row for row in CursorByName(cursor)]


def sort_entries(entries, sort_key='id'):
    """Sort a list of dictionaries by one their keys."""
    return sorted(entries, key=lambda k: k[sort_key])


# Tests

@patch('wazuh.core.utils.WazuhDBConnection', return_value=InitWDBSocketMock(sql_schema_file='schema_mitre_test.sql'))
def test_mitre_metadata(mock_mitre_dbmitre, mitre_db):
    """Check MITRE metadata."""
    result = mitre.mitre_metadata()
    rows = mitre_query(mitre_db, 'SELECT * FROM metadata')

    assert result.affected_items
    assert all(item[key] == row[key] for item, row in zip(result.affected_items, rows) for key in row)


@patch('wazuh.core.utils.WazuhDBConnection', return_value=InitWDBSocketMock(sql_schema_file='schema_mitre_test.sql'))
def test_mitre_tactics(mock_mitre_db, mitre_db):
    """Check MITRE tactics."""
    result = mitre.mitre_tactics()
    rows = mitre_query(mitre_db, "SELECT * FROM tactic")

    assert result.affected_items
    assert all(item[key] == row[key] for item, row in zip(sort_entries(result.affected_items), sort_entries(rows))
               for key in row)


@patch('wazuh.core.utils.WazuhDBConnection', return_value=InitWDBSocketMock(sql_schema_file='schema_mitre_test.sql'))
def test_mitre_techniques(mock_mitre_db, mitre_db):
    """Check MITRE groups."""
    result = mitre.mitre_techniques()
    rows = mitre_query(mitre_db, "SELECT * FROM technique")

    assert result.affected_items
    assert all(item[key] == row[key] for item, row in zip(sort_entries(result.affected_items), sort_entries(rows))
               for key in row)
