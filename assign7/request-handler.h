/**
 * File: request-handler.h
 * -----------------------
 * Defines the HTTPRequestHandler class, which fully proxies and
 * services a single client request.  
 */

#ifndef _request_handler_
#define _request_handler_

#include <utility>
#include <string>
#include "request.h"
#include "response.h"
#include <socket++/sockstream.h> // for sockbuf, iosockstream

class HTTPRequestHandler {
  public:
    void serviceRequest(const std::pair<int, std::string>& connection) throw();
    void clearCache();
    void setCacheMaxAge(long maxAge);

  private:
    HTTPRequest ingestRequest(std::istream& instream, const std::string& clientIPAddress);
    HTTPResponse ingestResponse(std::istream& instream);
    HTTPResponse createErrorResponse(int code, const std::string& message);
};

#endif
