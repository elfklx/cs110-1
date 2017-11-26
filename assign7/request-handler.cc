/**
 * File: request-handler.cc
 * ------------------------
 * Provides the implementation for the HTTPRequestHandler class.
 */

#include "request-handler.h"
#include "client-socket.h"
#include <unistd.h>
#include <netdb.h>
using namespace std;

void HTTPRequestHandler::serviceRequest(const pair<int, string>& connection) throw() {
  const int& clientfd = connection.first;
  const string& clientIPAddress = connection.second;
  sockbuf clientsb(clientfd);
  iosockstream clientStream(&clientsb);
  HTTPRequest request;
  try {
    request = ingestRequest(clientStream, clientIPAddress);
  } catch (const HTTPBadRequestException& hbre) {
    clientStream << createErrorResponse(400, hbre.what());
    clientStream.flush();
    return;
  }
  int serverfd = createClientSocket(request.getServer(), request.getPort());
  if (serverfd == kClientSocketError) {
    clientStream << createErrorResponse(404, "Server not found");
    clientStream.flush();
    return;
  }
  sockbuf serversb(serverfd);
  iosockstream serverStream(&serversb);
  serverStream << request;
  serverStream.flush();
  HTTPResponse response = ingestResponse(serverStream);
  clientStream << response;
  clientStream.flush();
}

// the following two methods needs to be completed 
// once you incorporate your HTTPCache into your HTTPRequestHandler
void HTTPRequestHandler::clearCache() {}
void HTTPRequestHandler::setCacheMaxAge(long maxAge) {}

HTTPRequest HTTPRequestHandler::ingestRequest(istream& instream, const string& clientIPAddress) {
  HTTPRequest request;
  request.ingestRequestLine(instream);
  request.ingestHeader(instream, clientIPAddress);
  request.ingestPayload(instream);
  request.addHeader("x-forwarded-proto", "http");
  request.addHeader("x-forwarded-for",
    request.getHeaderValueAsString("x-forwarded-for") + "," + clientIPAddress);
  return request;
}

HTTPResponse HTTPRequestHandler::ingestResponse(istream& instream) {
  HTTPResponse response;
  response.ingestResponseHeader(instream);
  response.ingestPayload(instream);
  return response;
}

HTTPResponse HTTPRequestHandler::createErrorResponse(int code, const string& message) {
  HTTPResponse response;
  response.setResponseCode(code);
  response.setProtocol("HTTP/1.0");
  response.setPayload(message);
  return response;
}