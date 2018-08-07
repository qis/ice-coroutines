#include "password.hpp"
#include <fstream>

std::string get_password()
{
  std::ifstream is("C:\\Workspace\\password.txt", std::ios::binary);
  std::string password;
  std::getline(is, password);
  return password;
}
