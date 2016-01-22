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


#ifndef _CONFIG_H_
#define _CONFIG_H_

#pragma once

#include <string>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <list>

#define TEMPLATES_DIRECTORY     "templates_directory"
#define RESOURCES_DIRECTORY     "resources_directory"
#define SERVER_WEB_PORT         "server_web_port"
#define SERVLET_LIST            "servlet_list"

namespace todo
{

  class servlet : public std::map<std::string, std::string>
  {
    friend class config;

  public:
    typedef std::list<servlet> servlet_list;
    std::string const & operator[](std::string const & p_key) const;

  private:
    std::string & operator()(std::string const & p_key);

  };


  class config : public std::map<std::string, std::string>
  {

  public:
    config(std::string const & p_filename);
    virtual ~config();

  public:
    bool                   parse_config();
    uint32_t               getServerPort() const;
    servlet::servlet_list const & getServlets() const;
    std::list<std::string> getListOfServlets() const;

  public:
    std::string const & operator[](std::string const & p_key) const;

  private:
    std::string &         operator()(std::string const & p_key);
    std::string           m_config_file;
    bool                  isKeyTrue(std::string const & p_key) const;
    void                  generateServletList();
    servlet::servlet_list m_servlets;

  };
}

#endif
