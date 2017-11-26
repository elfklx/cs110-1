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

HTTPRequestHandler::HTTPRequestHandler() throw (HTTPProxyException) {
  blacklist.addToBlacklist("blocked-domains.txt");
  cache = HTTPCache();
}

void HTTPRequestHandler::serviceRequest(const pair<int, string>& connection) throw() {
  const int& clientfd = connection.first;
  const string& clientIPAddress = connection.second;
  sockbuf clientsb(clientfd);
  iosockstream clientStream(&clientsb);
  HTTPRequest request;
  HTTPResponse response;
  try {
    ingestRequest(clientStream, clientIPAddress, request);
  } catch (const HTTPBadRequestException& hbre) {
    sendResponse(clientStream, createErrorResponse(400, hbre.what()));
    return;
  }
  if (!blacklist.serverIsAllowed(request.getServer())) {
    sendResponse(clientStream, createErrorResponse(403, "Forbidden Content"));
    return;
  }
  if (cache.containsCacheEntry(request, response)) {
    sendResponse(clientStream, response);
    return;
  }
  int serverfd = createClientSocket(request.getServer(), request.getPort());
  if (serverfd == kClientSocketError) {
    sendResponse(clientStream, createErrorResponse(404, "Server Not Found"));
    return;
  }
  sockbuf serversb(serverfd);
  iosockstream serverStream(&serversb);
  sendRequest(serverStream, request);
  ingestResponse(serverStream, response);
  if (cache.shouldCache(request, response)) {
    cache.cacheEntry(request, response);
  }
  sendResponse(clientStream, response);
}

void HTTPRequestHandler::clearCache() {
  cache.clear();
}

void HTTPRequestHandler::setCacheMaxAge(long maxAge) {
  cache.setMaxAge(maxAge);
}

void HTTPRequestHandler::ingestRequest(istream& instream,
  const string& clientIPAddress, HTTPRequest& request) {
  request.ingestRequestLine(instream);
  request.ingestHeader(instream, clientIPAddress);
  request.ingestPayload(instream);
  request.addHeader("x-forwarded-proto", "http");
  request.addHeader("x-forwarded-for",
    request.getHeaderValueAsString("x-forwarded-for") + "," + clientIPAddress);
}

void HTTPRequestHandler::ingestResponse(istream& instream,
  HTTPResponse& response) {
  response.ingestResponseHeader(instream);
  response.ingestPayload(instream);
}

HTTPResponse HTTPRequestHandler::createErrorResponse(int code, const string& message) {
  HTTPResponse response;
  response.setResponseCode(code);
  response.setProtocol("HTTP/1.0");
  response.setPayload(message);
  return response;
}

void HTTPRequestHandler::sendRequest(ostream& outstream, const HTTPRequest& request) {
  outstream << request;
  outstream.flush();
}

void HTTPRequestHandler::sendResponse(ostream& outstream, const HTTPResponse& response) {
  outstream << response;
  outstream.flush();
}