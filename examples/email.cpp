#include <wrappy/wrappy.h>

int main() {
     auto s = wrappy::call("smtplib.SMTP", "localhost");
     s.call("sendmail", "python@localhost", "wrappy@bmevers.de", 
         "Subject: Third time's the charm\n"
         "Dear Mr. or Ms.,\n"
         "please send my best regards.");
}
