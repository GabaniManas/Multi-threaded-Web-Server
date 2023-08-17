#include "http_server.hh"

#include <vector>

#include <sys/stat.h>

#include <fstream>
#include <sstream>
#include <string>
#include <time.h>


vector<string> split(const string &s, char delim) {
  vector<string> elems;

  stringstream ss(s);
  string item;

  while (getline(ss, item, delim)) {
    if (!item.empty())
      elems.push_back(item);
  }

  return elems;
}

HTTP_Request::HTTP_Request(string request) {
  vector<string> lines = split(request, '\n');
  vector<string> first_line = split(lines[0], ' ');

  this->HTTP_version = "1.0"; // We'll be using 1.0 irrespective of the request

  /*
   TODO : extract the request method and URL from first_line here
  */

  this->method = first_line[0];
  this->url = first_line[1];
  // this->HTTP_version = first_line[2];

  if (this->method != "GET") {
    cerr << "Method '" << this->method << "' not supported" << endl;
    exit(1);
  }
}

HTTP_Response *handle_request(string req) {
  HTTP_Request *request = new HTTP_Request(req);

  HTTP_Response *response = new HTTP_Response();

  string url = string("./html_files") + request->url;

  // response->HTTP_version = "1.0";
  response->HTTP_version = "1.1";

  struct stat sb;
  if (stat(url.c_str(), &sb) == 0) // requested path exists
  {
    response->status_code = "200";
    response->status_text = "OK";
    response->content_type = "text/html";

    string body;

    if (S_ISDIR(sb.st_mode)) {
      /*
      In this case, requested path is a directory.
      TODO : find the index.html file in that directory (modify the url
      accordingly)
      */
      ifstream index_html(url + "/index.html");
      getline(index_html, response->body, '\0');
      index_html.close();
    }

    /*
    TODO : open the file and read its contents
    */
    else {
      ifstream index_html(url);
      getline(index_html, response->body, '\0');
      index_html.close();
    }

    /*
    TODO : set the remaining fields of response appropriately
    */
    response->content_length = to_string(response->body.length());
  }

  else {
    response->status_code = "404";
    response->status_text = "Not Found";
    
    /*
    TODO : set the remaining fields of response appropriately
    */
    response->content_type = "text/html";
    // response->content_length = "0";

    response->body = "<html><head><title>Error 404 (Not Found)!!</title></head><body><h1>Requested page not found!</h1></body></html>";
    response->content_length = to_string(response->body.length());
  }

  // set Date field in response header
  char date_buf[30];
  time_t t;
  struct tm *tmp;
  const char* format = "%a, %d %b %Y %T GMT";

  t = time(NULL);
  tmp = gmtime(&t);
  if (tmp == NULL) {
    perror("gmtime error");
    exit(EXIT_FAILURE);
  }
  if (strftime(date_buf, sizeof(date_buf), format, tmp) == 0) {
    perror("strftime returned 0");
    exit(EXIT_FAILURE);
  }  
  response->date = date_buf;
  
  delete request;

  return response;
}

string HTTP_Response::get_string() {
  /*
  TODO : implement this function
  */
  string response;
  if (this->status_code == "200") {
    response.append("HTTP/" + this->HTTP_version + " " + this->status_code + " " + this->status_text + "\n");
    response.append("Date: " + this->date + "\n");
    response.append("Content-Type: " + this->content_type + "\n");
    response.append("Content-Length: " + this->content_length + "\n\n");
    response.append(this->body);
  } else {
    response.append("HTTP/" + this->HTTP_version + " " + this->status_code + " " + this->status_text + "\n");
    response.append("Date: " + this->date + "\n");
    response.append("Content-Type: " + this->content_type + "\n");
    response.append("Content-Length: " + this->content_length + "\n\n");
    response.append(this->body);
  }

 return response;
}