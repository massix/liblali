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


#include "config.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>

using namespace todo;

std::list<std::string> keys =
{
  TEMPLATES_DIRECTORY,
  RESOURCES_DIRECTORY,
  SERVER_WEB_PORT,
  SERVLET_LIST
};

config::~config()
{
}

config::config(std::string const & p_filename) :
  m_config_file(p_filename)
{
  // Sane defaults
  (*this)(TEMPLATES_DIRECTORY)      = "./templates/";
  (*this)(RESOURCES_DIRECTORY)      = "./resources/";
  (*this)(SERVER_WEB_PORT)          = "8080";
  (*this)(SERVLET_LIST)             = "default";
}

bool config::parse_config()
{
  bool l_ret(true);
  char l_line[1024];
  uint8_t l_index(0);

  struct stat l_stat;
  if (stat(m_config_file.c_str(), &l_stat) == -1)
    return false;

  // Open file
  std::ifstream l_file(m_config_file.c_str(), std::ifstream::in | std::ifstream::binary);

  // Read line per line
  while (not l_file.eof())
  {
    bzero(l_line, 1024);
    l_file.getline(l_line, 1024);
    if (l_line[0] == '#' or l_line[0] == ' ' or l_line[0] == '\0') continue;
    std::string l_string(l_line);
    l_ret = false;

    for (std::string const & l_key : keys) {
      if (l_string.substr(0, l_key.size()) == l_key) {
        if (l_key == SERVLET_LIST) {
          (*this)(l_key) = l_string.substr(l_string.find('=') + 2, l_string.size());
          generateServletList();
        }

        else if (l_key.substr(0, std::string("servlet_").size()) == "servlet_") {
          std::string l_servletName = l_key.substr(std::string("servlet_").size(), l_key.find_first_of(' '));
          std::string l_servletParam = l_servletName.substr(l_servletName.find_first_of('_') + 1, l_servletName.length());
          std::string l_servletValue = l_string.substr(l_string.find('=') + 2, l_string.size());
          l_servletName = l_servletName.substr(0, l_servletName.find_first_of('_'));

          for (servlet & c_servlet : m_servlets) {
            if (c_servlet["name"] == l_servletName) {
              c_servlet(l_servletParam) = l_servletValue;
              break;
            }
          }
        }

        else
          (*this)(l_key) = l_string.substr(l_string.find('=') + 2, l_string.size());
        l_ret = true;
      }
    }

    if (not l_ret) {
      // Uknown key, create a normal entry
      std::string l_genericKey;
      l_genericKey = l_string.substr(0, l_string.find(' '));
      (*this)(l_genericKey) = l_string.substr(l_string.find('=') + 2, l_string.size());
      l_ret = true;
    }

    l_index++;
  }

  return l_ret;
}

void config::generateServletList()
{
  std::string l_servletName;
  for (char const & n : (*this)[SERVLET_LIST]) {
    if (n == ' ') {
      servlet l_servlet;
      l_servlet("name") = l_servletName;
      m_servlets.push_back(l_servlet);

      keys.push_back(std::string("servlet_") + l_servletName + "_address");
      keys.push_back(std::string("servlet_") + l_servletName + "_db_file");
      keys.push_back(std::string("servlet_") + l_servletName + "_resources");
      keys.push_back(std::string("servlet_") + l_servletName + "_templates");
      keys.push_back(std::string("servlet_") + l_servletName + "_static");
      l_servletName.clear();
    }
    else {
      l_servletName += n;
    }
  }

  if (not l_servletName.empty()) {
    servlet l_servlet;
    l_servlet("name") = l_servletName;
    m_servlets.push_back(l_servlet);
    keys.push_back(std::string("servlet_") + l_servletName + "_address");
    keys.push_back(std::string("servlet_") + l_servletName + "_db_file");
    keys.push_back(std::string("servlet_") + l_servletName + "_resources");
    keys.push_back(std::string("servlet_") + l_servletName + "_templates");
    keys.push_back(std::string("servlet_") + l_servletName + "_static");
  }
}

std::list<std::string> config::getListOfServlets() const
{
  std::list<std::string> l_ret;
  for (servlet const & l_value : m_servlets) {
    l_ret.push_back(l_value["name"]);
  }

  return l_ret;
}

std::string const & config::operator[](std::string const & p_key) const
{
  return const_cast<std::string const &>(std::map<std::string, std::string>::at(p_key));
}

std::string & config::operator()(std::string const & p_key)
{
  return std::map<std::string, std::string>::operator[](p_key);
}

std::string const & servlet::operator[](std::string const & p_key) const
{
  return const_cast<std::string const &>(
    std::map<std::string, std::string>::at(p_key));
}

std::string & servlet::operator()(std::string const & p_key)
{
  return std::map<std::string, std::string>::operator[](p_key);
}

uint32_t config::getServerPort() const
{
  return atoi((*this)[SERVER_WEB_PORT].c_str());
}

bool config::isKeyTrue(std::string const & p_key) const
{
  bool l_res(false);
  std::string const & l_ask = (*this)[p_key];
  if (l_ask == "true" or l_ask == "ok" or l_ask == "yes")
    l_res = true;

  return l_res;
}

servlet::servlet_list const & config::getServlets() const
{
  return m_servlets;
}
