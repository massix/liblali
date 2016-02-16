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


#ifndef _WEB_H_
#define _WEB_H_

#include "http_request.h"
#include <string>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <functional>
#include "config.h"

namespace todo
{
  class web
  {
  public:
    typedef std::function<std::string(std::string const &, url::cgi_t const &, http_request &)> servlet_t;
    typedef std::map<std::string, servlet_t> servlets_t;
  public:
    web(config * p_config);
    virtual ~web();
    void run();
    void stop();

    typedef servlets_t::const_iterator const_iterator;
    const_iterator begin() const;
    const_iterator end() const;
    void insert(std::string const & p_key, servlet_t p_collection);

  private:
    bool get_content_of_file(std::string const & p_file, std::string & p_content, std::string & p_mime);

  private:
    uint32_t    m_port;
    std::string m_templates;
    std::string m_resources;
    servlets_t  m_servlets;
    int         m_socket;
    bool        m_running;
    config *    m_config;
  };
}

#endif
