from dataclasses import asdict
from unittest import mock

import pytest
from wazuh.core.indexer.commands import CommandsIndex, STATUS_KEY, TARGET_ID_KEY
from wazuh.core.indexer.base import IndexerKey, POST_METHOD, remove_empty_values
from wazuh.core.indexer.models.commands import Command, Source, Status, Type, CreateCommandResponse


class TestCommandsIndex:
    index_class = CommandsIndex
    create_command = Command(source=Source.ENGINE)

    @pytest.fixture
    def client_mock(self) -> mock.AsyncMock:
        return mock.AsyncMock()

    @pytest.fixture
    def index_instance(self, client_mock) -> CommandsIndex:
        return self.index_class(client=client_mock)

    async def test_create(self, index_instance: CommandsIndex, client_mock: mock.AsyncMock):
        """Check the correct functionality of the `create` method."""
        response = await index_instance.create(self.create_command)

        assert isinstance(response, CreateCommandResponse)
        client_mock.transport.perform_request.assert_called_once_with(
            method=POST_METHOD,
            url=index_instance.COMMAND_MANAGER_PLUGIN_URL,
            body=asdict(self.create_command, dict_factory=remove_empty_values),
        )

    async def test_get(self, index_instance: CommandsIndex, client_mock: mock.AsyncMock):
        """Check the correct functionality of the `get` method."""
        uuid = '0191dd54-bd16-7025-80e6-ae49bc101c7a'
        status = Status.PENDING
        query = {
            IndexerKey.QUERY: {
                IndexerKey.BOOL: {
                    IndexerKey.MUST: [
                        {IndexerKey.MATCH: {TARGET_ID_KEY: uuid}},
                        {IndexerKey.MATCH: {STATUS_KEY: status}},
                    ]
                }
            }
        }
        search_result = {IndexerKey.HITS: {IndexerKey.HITS: [
            {
                IndexerKey._ID: 'pBjePGfvgm',
                IndexerKey._SOURCE: {'target': {'id': uuid, 'type': Type.AGENT}, 'status': status}
            },
            {
                IndexerKey._ID: 'pBjePGfvgn',
                IndexerKey._SOURCE: {'target': {'id': '001', 'type': Type.AGENT}, 'status': status}
            },
        ]}}
        client_mock.search.return_value = search_result

        result = await index_instance.get(uuid=uuid, status=status)

        hits = search_result[IndexerKey.HITS][IndexerKey.HITS]
        assert result == [Command.from_dict(data[IndexerKey._ID], data[IndexerKey._SOURCE]) for data in hits]
        client_mock.search.assert_called_once_with(
            index=index_instance.INDEX,
            body=query,
        )
