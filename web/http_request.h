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


#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <string>
#include <map>
#include <vector>
#include <memory>

#ifndef __TESTABLE__
#define __TESTABLE__
#endif

namespace todo
{
  class url
  {
  public:
    typedef std::vector<std::string>           path_t;
    typedef std::map<std::string, std::string> cgi_t;

  public:
    url(std::string const & p_url);

  public:
    path_t const & get_path() const { return m_path; };
    std::string const get_full_path() const;
    std::string const & get_page() const { return m_page; };
    cgi_t const & get_cgi() const { return m_cgi; };
    void        parse_cgi(std::string const & p_cgi);

  private:
    path_t      m_path;
    cgi_t       m_cgi;
    std::string m_page;

    void        tokenize_path(std::string const & p_path);
  };

  class http_request
  {
  public:
    typedef std::map<std::string, std::string>  headers_t;
    typedef std::pair<std::string, std::string> header_t;
    typedef headers_t::const_iterator           const_iterator;
    typedef headers_t::iterator                 iterator;
    typedef headers_t::value_type               value_type;

    enum request_type_t {
      kGet  = 0,
      kPost = 1
    };

    enum response_code_t {
      kOkay = 200,
      kNotFound = 404
    };

  public:
    http_request(std::string const & p_request);
    http_request();
    virtual ~http_request();

  public:
    http_request::const_iterator begin() const;
    http_request::const_iterator end() const;
    http_request::iterator       begin();
    http_request::iterator       end();
    std::string & operator[](std::string const & p_key);

  public:
    void insert(header_t const & p_header);
    bool is_valid() const;
    std::string to_string() const;
    std::string code_to_string() const;

  public:
    std::shared_ptr<url> const & get_url() const { return m_url; };
    request_type_t       m_request;

  private:
    headers_t            m_headers;
    bool                 m_valid;
    std::shared_ptr<url> m_url;

  public:
    response_code_t      m_code;
  };
}

#endif
