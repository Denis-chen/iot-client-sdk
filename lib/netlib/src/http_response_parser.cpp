#include "http_response_parser.h"
#include <net/http_client.h>
#include <fmt/format.h>

namespace net
{
    namespace http
    {
        ResponseParser::ResponseParser(Response & response) : m_response(response), m_messageStarted(false), m_isComplete(false)
        {
            http_parser_init(&m_parser, HTTP_RESPONSE);
            m_parser.data = this;

            http_parser_settings_init(&m_callbacks);
            m_callbacks.on_message_begin = OnMessageBegin;
            m_callbacks.on_status = OnStatus;
            m_callbacks.on_header_field = OnHeaderField;
            m_callbacks.on_header_value = OnHeaderValue;
            m_callbacks.on_headers_complete = OnHeadersComplete;
            m_callbacks.on_body = OnBody;
            m_callbacks.on_message_complete = OnMessageComplete;
            m_callbacks.on_chunk_header = OnChunkHeader;
            m_callbacks.on_chunk_complete = OnChunkComplete;
        }

        void ResponseParser::Feed(const unsigned char * data, size_t len)
        {
            if (HasErrors())
            {
                return;
            }

            if (len == 0 && !m_messageStarted)
            {
                m_error.Set(-1, "Empty response received", "ResponseParser::Feed");
                return;
            }

            size_t parsed = http_parser_execute(&m_parser, &m_callbacks, reinterpret_cast<const char *>(data), len);
            if (parsed != len)
            {
                OnParseError(parsed, len);
            }
        }

        void ResponseParser::FeedEOF()
        {
            Feed(NULL, 0);
        }

        void ResponseParser::OnParseError(size_t pos, size_t len)
        {
            if (HasErrors())
            {
                return;
            }

            http_errno code = static_cast<http_errno>(m_parser.http_errno);
            const char *name = http_errno_name(code);
            const char *error = http_errno_description(code);
            std::string ctx = fmt::sprintf("pos=%d, len=%d", static_cast<int>(pos), static_cast<int>(len));
            m_error.Set(net::Status::Code(code, name), error, "http_parser_execute", ctx);
        }

        bool ResponseParser::IsComplete()
        {
            return m_isComplete || HasErrors();
        }

        bool ResponseParser::HasErrors()
        {
            return m_error.code != 0;
        }

        const net::Status & ResponseParser::GetLastError() const
        {
            return m_error;
        }

        int ResponseParser::OnMessageBegin()
        {
            m_messageStarted = true;
            return 0;
        }

        int ResponseParser::OnStatus(const char * at, size_t length)
        {
            m_response.httpStatus = static_cast<http::Status>(m_parser.status_code);
            return 0;
        }

        ResponseParser::Header::Header() : state(INITIAL) {}

        void ResponseParser::Header::AddTo(Response & response)
        {
            StringMap::iterator i = response.headers.find(name);
            if (i == response.headers.end())
            {
                response.headers[name] = value;
            }
            else
            {
                response.headers[name] += ", " + value;
            }
        }

        int ResponseParser::OnHeaderField(const char * at, size_t length)
        {
            switch (m_currentHeader.state)
            {
            case Header::LAST_WAS_FIELD:
                m_currentHeader.name.append(at, length);
                break;
            case Header::LAST_WAS_VALUE:
                m_currentHeader.AddTo(m_response);
            case Header::INITIAL:
                m_currentHeader.name.assign(at, length);
                m_currentHeader.state = Header::LAST_WAS_FIELD;
                break;
            default:
                return m_error.Set(-1, fmt::sprintf("Invalid m_currentHeader.state: %d", m_currentHeader.state), "OnHeaderField", std::string(at, length));
            }

            return 0;
        }

        int ResponseParser::OnHeaderValue(const char * at, size_t length)
        {
            switch (m_currentHeader.state)
            {
            case Header::LAST_WAS_FIELD:
                m_currentHeader.value.assign(at, length);
                m_currentHeader.state = Header::LAST_WAS_VALUE;
                break;
            case Header::LAST_WAS_VALUE:
                m_currentHeader.value.append(at, length);
                break;
            case Header::INITIAL:
                return m_error.Set(-1, "Missing OnHeaderField() call", "OnHeaderValue", std::string(at, length));
            default:
                return m_error.Set(-1, fmt::sprintf("Invalid m_currentHeader.state: %d", m_currentHeader.state), "OnHeaderValue", std::string(at, length));
            }

            return 0;
        }
        int ResponseParser::OnHeadersComplete()
        {
            switch (m_currentHeader.state)
            {
            case Header::INITIAL:
                break;
            case Header::LAST_WAS_VALUE:
                m_currentHeader.AddTo(m_response);
                m_currentHeader.state = Header::INITIAL;
                break;
            case Header::LAST_WAS_FIELD:
                return m_error.Set(-1, "Missing OnHeaderValue() call", "OnHeadersComplete", "");
            default:
                return m_error.Set(-1, fmt::sprintf("Invalid m_currentHeader.state: %d", m_currentHeader.state), "OnHeadersComplete", "");
            }

            ReserveData();
            return 0;
        }

        int ResponseParser::OnBody(const char * at, size_t length)
        {
            m_response.data.append(at, length);
            return 0;
        }

        int ResponseParser::OnMessageComplete()
        {
            m_isComplete = true;
            return 0;
        }

        int ResponseParser::OnChunkHeader()
        {
            ReserveData();
            return 0;
        }

        int ResponseParser::OnChunkComplete()
        {
            return 0;
        }

        void ResponseParser::ReserveData()
        {
            // Check if Content-Length/chunk size is available, and reserve (additional) space in response.data
            if (m_parser.content_length > 0 && m_parser.content_length != ULLONG_MAX)
            {
                m_response.data.reserve(m_response.data.size() + m_parser.content_length);
            }
        }

        ResponseParser * ResponseParser::ThisFrom(http_parser * parser)
        {
            return reinterpret_cast<ResponseParser *>(parser->data);
        }

        int ResponseParser::OnMessageBegin(http_parser * parser)
        {
            return ThisFrom(parser)->OnMessageBegin();
        }

        int ResponseParser::OnStatus(http_parser * parser, const char * at, size_t length)
        {
            return ThisFrom(parser)->OnStatus(at, length);
        }

        int ResponseParser::OnHeaderField(http_parser * parser, const char * at, size_t length)
        {
            return ThisFrom(parser)->OnHeaderField(at, length);
        }

        int ResponseParser::OnHeaderValue(http_parser * parser, const char * at, size_t length)
        {
            return ThisFrom(parser)->OnHeaderValue(at, length);
        }

        int ResponseParser::OnHeadersComplete(http_parser * parser)
        {
            return ThisFrom(parser)->OnHeadersComplete();
        }

        int ResponseParser::OnBody(http_parser * parser, const char * at, size_t length)
        {
            return ThisFrom(parser)->OnBody(at, length);
        }

        int ResponseParser::OnMessageComplete(http_parser * parser)
        {
            return ThisFrom(parser)->OnMessageComplete();
        }

        int ResponseParser::OnChunkHeader(http_parser * parser)
        {
            return ThisFrom(parser)->OnChunkHeader();
        }

        int ResponseParser::OnChunkComplete(http_parser * parser)
        {
            return ThisFrom(parser)->OnChunkComplete();
        }
    }
}
