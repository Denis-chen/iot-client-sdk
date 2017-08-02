#pragma once

#include <net/common.h>
#include <http_parser.h>
#include <string>

namespace net
{
    namespace http
    {
        class Response;

        class ResponseParser
        {
        public:
            ResponseParser(Response& response);
            void Feed(const unsigned char *data, size_t len);
            void FeedEOF();
            bool IsComplete();
            bool HasErrors();
            const net::Status& GetLastError() const;

        private:
            void OnParseError(size_t pos, size_t len);
            int OnMessageBegin();
            int OnStatus(const char *at, size_t length);
            int OnHeaderField(const char *at, size_t length);
            int OnHeaderValue(const char *at, size_t length);
            int OnHeadersComplete();
            int OnBody(const char *at, size_t length);
            int OnMessageComplete();
            int OnChunkHeader();
            int OnChunkComplete();
            void ReserveData();

            static ResponseParser * ThisFrom(http_parser *parser);
            static int OnMessageBegin(http_parser *parser);
            static int OnStatus(http_parser *parser, const char *at, size_t length);
            static int OnHeaderField(http_parser *parser, const char *at, size_t length);
            static int OnHeaderValue(http_parser *parser, const char *at, size_t length);
            static int OnHeadersComplete(http_parser* parser);
            static int OnBody(http_parser *parser, const char *at, size_t length);
            static int OnMessageComplete(http_parser* parser);
            static int OnChunkHeader(http_parser* parser);
            static int OnChunkComplete(http_parser* parser);

            class Header
            {
            public:
                Header();
                void AddTo(Response& response);
                std::string name;
                std::string value;
                enum State { INITIAL, LAST_WAS_FIELD, LAST_WAS_VALUE } state;
            };

            Response& m_response;
            http_parser m_parser;
            http_parser_settings m_callbacks;
            Header m_currentHeader;
            bool m_messageStarted;
            bool m_isComplete;
            net::Status m_error;
        };
    }
}
