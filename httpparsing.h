#include "config.h"
#include "definitions.h"
#include <iostream>

using namespace Config;
using namespace Definitions;

class HTTPParser {
private:

public:
    int HeaderEnd(std::string acc){
        if(acc.length() > REQ_HEADER_CAP){ return TOO_LARGE; }
        auto header_end_index = acc.find("\r\n\r\n");
        if(header_end_index == std::string::npos){ return COULD_NOT_FIND; }
        return header_end_index;
    }

    bool ContainsTE(std::string acc, int header_end_index){
        auto TE_index = acc.find("Transfer-Encoding", header_end_index);
        if(TE_index != std::string::npos){ return true; }
        return false;
    }

    int BodyLength(std::string acc, int header_end_index){
        auto CL_index = acc.rfind("Content-Length", header_end_index);
        if(CL_index == std::string::npos){ return COULD_NOT_FIND; }
        auto CL_end_index = acc.find("\r\n", CL_index);
        if(CL_end_index == std::string::npos){ return COULD_NOT_FIND; }
        try{
            int bytes = std::stoi(acc.substr(CL_index + 16, CL_end_index)); // stoi may throw an error
            if(bytes > REQ_BODY_CAP){ return TOO_LARGE; }
            return bytes;
        } catch(...){
            return THREW_ERROR;
        }
    }

    // Checking for headers even after theyre found is unecessary. Optimise this later
    int RequestLength(std::string acc){
        int header_end = HeaderEnd(acc);
        if(header_end == COULD_NOT_FIND){ return CONTINUE; }
        if(header_end == TOO_LARGE){ return CLOSE_ERASE_CONTINUE; }
        if(ContainsTE(acc, header_end)){ return CLOSE_ERASE_CONTINUE; }

        int body_length = BodyLength(acc, header_end);
        if(body_length == TOO_LARGE){ return CLOSE_ERASE_CONTINUE; }
        if(body_length == THREW_ERROR){ return CLOSE_ERASE_CONTINUE; }
        if(body_length == COULD_NOT_FIND){ return 0; }

        const int req_length = header_end + body_length + 4;
        if(acc.length() < req_length){ return CONTINUE; }
        if(acc.length() - header_end - 4 > REQ_BODY_CAP){ return CLOSE_ERASE_CONTINUE; }

        return req_length;
    }
};

HTTPParser parser;