//
//           DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//                   Version 2, December 2004
//
// Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
//
// Software :
// Copyright (C) 2014 Massimo Gengarelli <massimo.gengarelli@gmail.com>
//
// Everyone is permitted to copy and distribute verbatim or modified
// copies of this license document, and changing it is allowed as long
// as the name is changed.
//
//            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//
//  0. You just DO WHAT THE FUCK YOU WANT TO.
//


#include "web.h"
#include "http_request.h"
#include "config.h"
#include <flate.h>
#include <json11.hpp>

#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include <iostream>
#include <fstream>
#include <unistd.h>

using namespace todo;

web::~web()
{
  closelog();
}

web::web(config * p_config) :
  m_port(p_config->getServerPort()), m_socket(0), m_running(false), m_config(p_config)
{
  // Init the socket, for now we don't care whether the socket is valid or not
  m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  m_templates = (*p_config)[TEMPLATES_DIRECTORY];
  m_resources = (*p_config)[RESOURCES_DIRECTORY];

  // Open the syslog connection
  openlog("lali-web", LOG_PID | LOG_NOWAIT, LOG_USER);

  // Register a dump configuration servlet
  m_servlets["/debug/"] = [&](std::string const & p_page, url::cgi_t const & p_cgi, http_request & p_request)->std::string {
    std::string l_ret;
    std::string l_contentType = "text/html";
    p_request.m_code = http_request::kOkay;

    if (p_page.empty()) {
      Flate * l_flate = NULL;
      flateSetFile(&l_flate, std::string(m_templates + "debug_template.html").c_str());

      for (config::value_type const & c_key : (*m_config)) {
        flateSetVar(l_flate, "key", c_key.first.c_str());
        flateSetVar(l_flate, "value", c_key.second.c_str());
        flateDumpTableLine(l_flate, "config");
      }

      for (url::cgi_t::value_type const & l_value : p_cgi) {
        if (l_value.first != "submit") {
          flateSetVar(l_flate, "cgi_key", l_value.first.c_str());
          flateSetVar(l_flate, "cgi_value", l_value.second.c_str());
          flateDumpTableLine(l_flate, "cgi");
        }
      }

      for (servlet const & l_servlet : m_config->getServlets()) {
        flateSetVar(l_flate, "servlet_name", l_servlet["name"].c_str());

        for (servlet::value_type const & l_value : l_servlet) {
          flateSetVar(l_flate, "servlet_key", l_value.first.c_str());
          flateSetVar(l_flate, "servlet_value", l_value.second.c_str());
          flateDumpTableLine(l_flate, "servlet_config");
        }

        flateDumpTableLine(l_flate, "servlet_container");
      }

      l_ret = flatePage(l_flate);
    }
    else if (not get_content_of_file(m_templates + p_page, l_ret, l_contentType)) {
      p_request.m_code = http_request::kNotFound;
      l_ret = "Page not found";
    }

    p_request["Content-Type"] = l_contentType;
    return l_ret;
  };

  // Resources
  m_servlets["/resources/"] = [&](std::string const & p_page, url::cgi_t const & /*p_cgi*/, http_request & p_request)->std::string {
    std::string l_resource = m_resources + p_page;

    std::string l_contentType;
    std::string str;

    p_request.m_code = http_request::kOkay;

    if (not get_content_of_file(l_resource, str, l_contentType)) {
      p_request.m_code = http_request::kNotFound;
    }

    p_request["Content-Type"] = l_contentType;
    return str;
  };
}

void web::insert(const std::string &p_key, servlet_t p_collection)
{
  m_servlets[p_key] = p_collection;
}

bool web::get_content_of_file(std::string const & p_file, std::string & p_content, std::string & p_mime)
{
  struct stat l_stat;

  if (stat(p_file.c_str(), &l_stat) == -1)
    return false;

  std::ifstream l_file(p_file.c_str(), std::ifstream::in | std::ifstream::binary);

  l_file.seekg(0, std::ios::end);
  p_content.resize(l_file.tellg());
  l_file.seekg(0, std::ios::beg);

  p_content.assign((std::istreambuf_iterator<char>(l_file)),
                   std::istreambuf_iterator<char>());


  typedef std::map<std::string, std::string> t_mime_type_map;
  t_mime_type_map l_mime_types = {
    {"css"  , "text/css" },
    {"js"   , "application/javascript" },
    {"html" , "text/html" },
    {"htm"  , "text/html" },
    {"png"  , "image/png" }
  };

  p_mime = "text/plain";
  for (t_mime_type_map::value_type const & l_pair : l_mime_types) {
    if (p_file.substr(p_file.size() - l_pair.first.size(), p_file.size()) == l_pair.first) {
      p_mime = l_pair.second;
      break;
    }
  }

  return true;
}

void web::stop()
{
  m_running = false;
}

void web::run()
{
  if (m_socket)
  {
    struct sockaddr_in l_serverAddress;

    bzero(&l_serverAddress, sizeof(l_serverAddress));

    l_serverAddress.sin_family = AF_INET;
    l_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    l_serverAddress.sin_port = htons(m_port);

    // Set a timeout
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 1000;

    // Reusable socket
    int yes = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
      syslog(LOG_ERR, "Impossible to create a socket");
      return;
    }

    // Bind the socket
    if (bind(m_socket, (struct sockaddr *) &l_serverAddress, sizeof(l_serverAddress)) == -1)
    {
      syslog(LOG_ERR, "Cannot bind to address");
      return;
    }

#ifdef SO_REUSEPORT
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int));
#endif

    // Listen
    if (listen(m_socket, 3) == -1)
    {
      syslog(LOG_ERR, "Cannot listen on given port");
      return;
    }

    m_running = true;
    while (m_running) {
      fd_set l_set;

      FD_ZERO(&l_set);
      FD_SET(m_socket, &l_set);

      select(m_socket + 1, &l_set, nullptr, nullptr, &tv);

      if (FD_ISSET(m_socket, &l_set) and m_running) {
        std::thread l_thread([&] {
          struct sockaddr_in l_clientAddress;
          bzero(&l_clientAddress, sizeof(l_clientAddress));
          std::string l_request;
          int l_clientLength = sizeof(l_clientAddress);
          l_request.resize(4096);

          int l_accepted = accept(m_socket,
                                  (struct sockaddr *) &l_clientAddress,
                                  (socklen_t *) &l_clientLength);

          char * l_clientIP = inet_ntoa(l_clientAddress.sin_addr);

          int32_t l_length = recv(l_accepted, (void *) l_request.data(), l_request.size(), 0);
          l_request.resize(l_length);

          // If we are here, we have a request to handle
          http_request l_headers(l_request);
          syslog(LOG_NOTICE, "Received request from %s [ %s ]",
                 l_clientIP, l_headers["HTTP_Request"].c_str());

          // Check if we are waiting for a body too (POST)
          if (l_headers.m_request == http_request::kPost) {
            // Set non-blocking socket in order to avoid infinite wait
            int flags;
            flags = fcntl(l_accepted, F_GETFL, 0);
            fcntl(l_accepted, F_SETFL, flags | O_NONBLOCK);

            l_request.resize(atoi(l_headers["Content-Length"].c_str()));
            l_length = recv(l_accepted, (void *) l_request.data(), l_request.size(), 0);
            syslog(LOG_DEBUG, "Length of body: %d", l_length);

            if (l_length != -1) {
              syslog(LOG_DEBUG, "Received body: %s", l_request.c_str());
              l_headers.get_url()->parse_cgi(l_request);
            }
          }

          // Call the servlet if we have it
          std::string l_html_response;
          http_request l_responseHeaders;
          l_responseHeaders.m_request = l_headers.m_request;
          l_responseHeaders["Content-Type"] = "text/html";
          l_responseHeaders["Connection"] = "close";

          std::string l_calledServlet;
          std::string l_askedPage;

          if (not l_headers.get_url()->get_path().empty()) {
            l_calledServlet = "/" + l_headers.get_url()->get_path()[0] + "/";
            l_askedPage = l_headers.get_url()->get_full_path().substr(l_calledServlet.size());
          }
          else {
            l_calledServlet = l_headers.get_url()->get_full_path();
          }

          l_askedPage += l_headers.get_url()->get_page();

          if (m_servlets.find(l_calledServlet) != m_servlets.end())
          {
            l_html_response = m_servlets[l_calledServlet](l_askedPage,
                                                          l_headers.get_url()->get_cgi(),
                                                          l_responseHeaders);
          }
          else
          {
            l_responseHeaders.m_code = http_request::kNotFound;
            Flate * l_flate = NULL;

            flateSetFile(&l_flate, std::string(m_templates + "404_template.html").c_str());
            flateSetVar(l_flate, "servlet",
                        l_headers.get_url()->get_full_path().c_str());
            flateSetVar(l_flate, "page",
                        l_headers.get_url()->get_page().c_str());

            char * l_buffer = flatePage(l_flate);
            l_html_response = l_buffer;
          }


          l_responseHeaders["Server"] = "lali-web-dev";
          l_responseHeaders["Content-Length"] = std::to_string(l_html_response.size());

          std::string l_response = l_responseHeaders.to_string();
          l_response += l_html_response;

          l_length = send(l_accepted,
                          l_response.c_str(),
                          l_response.size(),
                          0);

          syslog(LOG_NOTICE, "Replying to %s [ %s ]",
                 l_clientIP, l_responseHeaders.code_to_string().c_str());
          close(l_accepted);
        });

        // Fork it in background
        l_thread.detach();
      }
    }

    syslog(LOG_NOTICE, "[%p] closing", this);
    close(m_socket);
  }
}
