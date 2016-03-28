#ifndef __PARTICLE_FTP_CLIENT
#define __PARTICLE_FTP_CLIENT

#include "application.h"
#include <time.h>

#define RESPONSE_BUFFER_SIZE 270

namespace particleftpclient {
  class ParticleFtpClient {
      public:
          // Open a conncetion to hostname. Timeout is a global timeout that will
          // be the number of seconds the client will wait for any
          // command to complete.
          bool open(String hostname, int timeout);

          // Sends a username, returns true if the username was accepted
          bool user(String username);

          // Sends a password, returns true if the password was accepted
          bool pass(String password);

          // Returns true if the FTP connection is open
          bool connected();

          /************* Common FTP commands *************************
           *  The following methods are standard ftp commands. For
           *  methods that return a  bool, they will simply return
           *  true if they are executed successfully or false if they
           *  fail.
           ***********************************************************/

          // Gets size of a filename, or -1 if an error occurs
          int size(String filename);

          // Reads the modified date and time and stores it in the
          // supplied tm struct. Returns true if successful.
          bool mdtm(String filename, tm* mdtm_time);

          // Returns a String with the a response to the
          // "pwd" command, which prints the current working
          // directory. Since the format of this string is dependent
          // on the operating system and FTP server, the String will
          // be the response of the command, including the
          // result code.
          String pwd();


          // Changes working directory
          bool cwd(String directory);

          // Makes a new directory
          bool mkd(String dirname);

          // Removes a directory
          bool rmd(String dirname);

          // Sets the file tranfer type. Typical types are "A" for ASCII text or "I" for binary images
          bool type(String typestring);

          // Deletes a file
          bool dele(String filename);

          // Goes up to the parent directory
          bool cdup();

          // Logs out of the FTP server
          bool quit();


          /************* TCPClient data operations *******************
           *  The following methods will establish a data stream
           *  connection. If the command is valid, true will be
           *  returned and The TCPClient data field will be connected
           *  for reading or writing the appropriate data. Otherwise,
           *  false will be returned and data.connected() will also
           *  return false (with the stream being unreadable and
           *  unwriteable)
           ***********************************************************/

          // Lists files matching the filespec. Data is returned via the data TCPClient, or false if
          // an error occured.
          bool list(String filespec);

          // Stores data in the specified filename. Data can be written with the data TCPClient.
          // False will be returned if an error occured
          bool stor(String filename);

          // Not an actual FTP command, but closes the data port after a STOR command
          // and returns true if the connection closed correctly
          bool finish();

          // Retrieves the specified filename. Data can be read from the data TCPClient.
          // False will be returned if an error occured
          bool retr(String filename);

          // Aborts any data transfer in progress and closes the data connection
          bool abor();

          // This TCPClient instance will be readable or writeable if the above methods
          // are called and return true
          TCPClient data;

          bool simple_command(String cmd, int successCode);
          String get_response();
          bool pasv_command(String cmd, int successCode);

      protected:
          TCPClient server_cmd_connection;

          bool connect_data_port();
          int parse_response();

          char cmd_response_buffer[RESPONSE_BUFFER_SIZE];
          int maxTries = 10;
  };
}

#endif
