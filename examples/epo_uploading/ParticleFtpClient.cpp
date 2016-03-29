#include "ParticleFtpClient.h"
#include "application.h"
#include <time.h>

namespace particleftpclient {

  // #define PARTICLE_FTP_DEBUG

  #ifdef PARTICLE_FTP_DEBUG
  #define d_print(message)        Serial.print(String(message))
  #define d_println(message)      Serial.println(String(message))
  #else
  #define d_print(message)        ;
  #define d_println(message)      ;
  #endif

  #define expect(code)            if (code / 100 != parse_response() / 100) return false;

  bool ParticleFtpClient::open(String hostname, int timeout) {
      maxTries = timeout;
  #ifdef PARTICLE_FTP_DEBUG
      Serial.begin(9600);
  #endif
      d_println("Connecting...");
      server_cmd_connection.connect(hostname, 21);
      if (!server_cmd_connection.connected()) {
          d_println("Failed to connect to command port");
          return false;
      }
      d_println("Waiting for welcome message");
      expect(200);
      d_println("Welcome received");
      server_cmd_connection.flush(); // Get rid of annoying welcome messages
      return true;
  }

  bool ParticleFtpClient::user(String username) {
    return simple_command("USER " + username, 300);
  }

  bool ParticleFtpClient::pass(String password) {
    return simple_command("PASS " + password, 200);
  }

  bool ParticleFtpClient::connected() {
      return server_cmd_connection.connected();
  }

  int ParticleFtpClient::size(String filename) {
      server_cmd_connection.println("SIZE " + filename);
      int result = parse_response();
      if (result / 100 != 2) return -1;
      return atoi(&cmd_response_buffer[4]);
  }

  bool ParticleFtpClient::mdtm(String filename, tm *mdtm_time) {
      server_cmd_connection.println("MDTM " + filename);
      expect(200);
      char yearstr[5] = { 0 };
      strncpy(yearstr, &cmd_response_buffer[4], 4);
      char monthstr[3] = { 0 };
      strncpy(monthstr, &cmd_response_buffer[8], 2);
      char daystr[3] = { 0 };
      strncpy(daystr, &cmd_response_buffer[10], 2);
      char hhstr[3] = { 0 };
      strncpy(hhstr, &cmd_response_buffer[12], 2);
      char mmstr[3] = { 0 };
      strncpy(mmstr, &cmd_response_buffer[14], 2);
      char ssstr[3] = { 0 };
      strncpy(ssstr, &cmd_response_buffer[16], 2);
      mdtm_time->tm_sec = atoi(ssstr);
      mdtm_time->tm_min = atoi(mmstr);
      mdtm_time->tm_hour = atoi(hhstr);
      mdtm_time->tm_mday = atoi(daystr);
      mdtm_time->tm_mon = atoi(monthstr);
      mdtm_time->tm_year = atoi(yearstr);
      return true;
  }

  String ParticleFtpClient::pwd() {
      server_cmd_connection.println("PWD");
      int result = parse_response();
      if (result / 100 == 200 / 100) return String("");
      return String(cmd_response_buffer);
  }

  bool ParticleFtpClient::cwd(String dirname) {
      return simple_command("CWD " + dirname, 200);
  }

  bool ParticleFtpClient::mkd(String dirname) {
      return simple_command("MKD " + dirname, 200);
  }

  bool ParticleFtpClient::rmd(String dirname) {
      return simple_command("RMD " + dirname, 200);
  }

  bool ParticleFtpClient::type(String typestring) {
      return simple_command("TYPE " + typestring, 200);
  }

  bool ParticleFtpClient::dele(String filename) {
      return simple_command("DELE " + filename, 200);
  }

  bool ParticleFtpClient::cdup() {
      return simple_command("CDUP", 200);
  }

  bool ParticleFtpClient::quit() {
      return simple_command("QUIT", 200);
  }

  bool ParticleFtpClient::list(String filespec) {
      return pasv_command("LIST " + filespec, 150);
  }

  bool ParticleFtpClient::stor(String filename) {
      return pasv_command("STOR " + filename, 150);
  }

  bool ParticleFtpClient::finish() {
      data.stop();
      expect(200);
      return true;
  }

  bool ParticleFtpClient::retr(String filename) {
      return pasv_command("RETR " + filename, 150);
  }

  bool ParticleFtpClient::abor() {
      return simple_command("ABOR", 200);
  }

  bool ParticleFtpClient::simple_command(String cmd, int successCode) {
      server_cmd_connection.println(cmd);
      expect(successCode);
      return true;
  }

  String ParticleFtpClient::get_response() {
      return String(cmd_response_buffer);
  }

  bool ParticleFtpClient::pasv_command(String cmd, int successCode) {
      if (!connect_data_port()) return false;
      return simple_command(cmd, successCode);
  }

  bool ParticleFtpClient::connect_data_port() {
      server_cmd_connection.println("PASV");
      int tries = 0;
      int index = 0;
      memset(cmd_response_buffer, 0, RESPONSE_BUFFER_SIZE);
      bool responded = false;
      d_print("PASV response: ");
      while (tries < maxTries && !responded) {
          delay(1000);

          // Receive as much as we can from the buffer
          while (server_cmd_connection.available() && index < RESPONSE_BUFFER_SIZE) {
              char val = server_cmd_connection.read();
              cmd_response_buffer[index++] = val;
              d_print(val);
          }

          // Trap for a failure result_code for pasv command, assuming enough chars come through
          if (index >= 4) {
              char code_string[4] = { 0 };
              memcpy(code_string, cmd_response_buffer, 3);
              if (atoi(code_string) / 100 != 2) {
                  return false;
              }
          }

          // Make sure we received a full PASV response with closing parentheses
          for (unsigned int i = 0; i < strlen(cmd_response_buffer); i++) {
              if (cmd_response_buffer[i] == ')') {
                  responded = true;
              }
          }
      }

      // Parse pasv IP address and port number
      int pasv_vals[6];
      char* pointer = strtok(cmd_response_buffer, "(,)");
      for (int i = 0; i < 6; i++) {
          pointer = strtok(NULL, "(,)");
          if (pointer == NULL) {
              d_println("Couldn't parse PASV info");
              return false;
          }
          pasv_vals[i] = atoi(pointer);
      }
      IPAddress pasv_ip(pasv_vals[0], pasv_vals[1], pasv_vals[2], pasv_vals[3]);
      d_print("PASV address ");
      d_print(pasv_ip);
      d_print(":");
      d_println(pasv_vals[4] * 256 + pasv_vals[5]);
      data.stop();
      data.flush();
      data.connect(pasv_ip, pasv_vals[4] * 256 + pasv_vals[5]);

      if (!data.connected()) {
          d_println("Could not connect to PASV port!");
      }
      return data.connected();
  }

  int ParticleFtpClient::parse_response() {
      int tries = 0;
      uint8_t index = 0;
      memset(cmd_response_buffer, 0, RESPONSE_BUFFER_SIZE);
      bool responded = false;
      d_print("Server response: ");
      while (tries < maxTries && !responded) {
          delay(1000);
          while (server_cmd_connection.available() && index < RESPONSE_BUFFER_SIZE) {
              tries = 0;
              char val = server_cmd_connection.read();
              cmd_response_buffer[index++] = val;
              responded = index > 3;
              d_print(val);
          }
          tries++;
      }
      if (tries >= maxTries) d_println("Server timed out on response");
      d_println();
      if (!responded) return 0;
      char code_string[4] = { 0 };
      memcpy(code_string, cmd_response_buffer, 3);
      server_cmd_connection.flush();

      return atoi(code_string);
  }
}
