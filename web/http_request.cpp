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


#include "http_request.h"
#include <string>
#include <syslog.h>

using namespace todo;

url::url(std::string const & p_url)
{
  bool l_hasCGI = (p_url.find_first_of('?') != std::string::npos);

  if (l_hasCGI) {
    tokenize_path(p_url.substr(0, p_url.find_first_of('?')));
    parse_cgi(p_url.substr(p_url.find_first_of('?') + 1));
  }

  else tokenize_path(p_url);
}

void url::tokenize_path(std::string const & p_path)
{
  std::string l_path;
  for (const char & n : p_path)
  {
    if (n == '/') {

      if (not l_path.empty() and l_path != "/") {
        m_path.push_back(l_path);
        l_path.clear();
      }

      continue;
    }

    l_path += n;
  }

  if (not l_path.empty() and l_path != "/") {
    m_page = l_path;
  }
}

std::string const url::get_full_path() const
{
  std::string l_path("/");

  for (std::string const & c : m_path) {
    l_path += c + "/";
  }

  return l_path;
}

void url::parse_cgi(std::string const & p_cgi)
{
  bool l_value(false);
  bool l_key(true);

  std::string l_keyValue;
  std::string l_valueValue;

  for (const char & n : p_cgi)
  {
    if (n == '?') continue;

    else if (n == '&') {
      l_value = false;
      l_key = true;
      m_cgi[l_keyValue] = l_valueValue;
      l_valueValue.clear();
      l_keyValue.clear();
      continue;
    }
    else if (n == '=') {
      l_value = true;
      l_key = false;
      continue;
    }

    if (l_key) l_keyValue += n;
    else if (l_value) l_valueValue += n;
  }

  m_cgi[l_keyValue] = l_valueValue;
}

http_request::http_request() : m_valid(true), m_code(kOkay)
{
}

http_request::http_request(std::string const & p_request) : m_valid(false)
{
  bool l_value(false);
  bool l_key(true);

  std::string l_keyValue;
  std::string l_valueValue;
  std::string l_request;
  l_request = p_request.substr(0, p_request.find_first_of('\r'));
  std::string l_rest = p_request.substr(p_request.find_first_of('\n'));

  // Special header to store the HTTP_Request
  m_headers["HTTP_Request"] = l_request;

  std::string l_method = l_request.substr(0, l_request.find_first_of(' '));
  std::string l_url = l_request.substr(l_request.find_first_of('/'));
  l_url = l_url.substr(0, l_url.find_first_of(' '));
  std::string l_protocol = l_request.substr(l_request.find_last_of(' ') + 1);
  std::string l_last_line;

  m_request = kGet;
  if (l_method == "POST")
  {
    m_request = kPost;
  }

  // We can deduce the URL will be within the two fixed parts
  if (m_valid)
    m_headers["URL"] = l_url;

  m_url.reset(new url(l_url));

  // The stupidest parsing ever.
  for (const char & n : l_rest)
  {
    // Here we have finished parsing a key
    if (n == ':') {
      l_value = true;
      l_key = false;
      continue;
    }

    // Here we have finished parsing a value, hence we have everything
    else if (n == '\r' or n == '\n') {
      l_value = false;
      l_key = true;

      // We don't care about doing it twice, this is a map.
      if (not l_keyValue.empty())
        // the first character is empty, avoid it
        m_headers[l_keyValue] = l_valueValue.substr(1);

      l_keyValue.clear();
      l_valueValue.clear();
      l_last_line.clear();
      continue;
    }

    l_last_line += n;
    if (l_key) l_keyValue += n;
    if (l_value) l_valueValue += n;
  }

  syslog(LOG_DEBUG, "Full request was\n%s\n", p_request.c_str());
  if (not l_last_line.empty())
    m_url->parse_cgi(l_last_line);
}

http_request::~http_request()
{
}

/* Getters and setters woohoo! */
bool http_request::is_valid() const
{
  return m_valid;
}

http_request::const_iterator http_request::begin() const
{
  return m_headers.begin();
}

http_request::const_iterator http_request::end() const
{
  return m_headers.end();
}

http_request::iterator http_request::begin()
{
  return m_headers.begin();
}

http_request::iterator http_request::end()
{
  return m_headers.end();
}

void http_request::insert(header_t const & p_header)
{
  m_headers[p_header.first] = p_header.second;
}

std::string & http_request::operator[](std::string const & p_key)
{
  return m_headers[p_key];
}

std::string http_request::to_string() const
{
  std::string l_ret = code_to_string();
  l_ret += "\r\n";

  for (value_type const & c : (*this))
      l_ret += c.first + ": " + c.second + "\r\n";

  l_ret += "\r\n";

  return l_ret;
}

std::string http_request::code_to_string() const
{
  std::string l_ret("HTTP/1.1 ");
  switch (m_code) {
  case kOkay:
    l_ret += "200 Okay";
    break;
  case kNotFound:
    l_ret += "404 Not Found";
    break;
  }

  return l_ret;
}
