#include <ParseCoordinates.hpp>
#include <regex>
#include <cmath>
#include <cstdlib>
#include <filesystem>

int progressCount = 0;
int total = 0;

void showProgressBar(size_t barWidth = 50) {
    double progress = static_cast<double>(progressCount) / (total);
    size_t pos = static_cast<size_t>(barWidth * progress);

    std::cout << "[";
    for (size_t i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    if (progress < 1.0) {
        std::cout.flush();
    } else {
        std::cout << std::endl;
    }
}

std::ostream& operator<<(std::ostream& stream, const Coordinate& coord){
    if (coord.latitutde == LATITUDE::NORTH){
        stream << "N:";
    }
    else if(coord.latitutde == LATITUDE::SOUTH){
        stream << "S:";
    }
    else{
        return stream;
    }

    stream << coord.latitude_degree << " | ";

    if(coord.longitude == LONGITUDE::EAST){
        stream << "E:";
    }
    else if(coord.longitude == LONGITUDE::WEST){
        stream << "W:";
    }
    else{
        return stream;
    }

    stream << coord.longitude_degree << "\n";
    return stream;
}

Coordinate stringToCoordinate(const std::string& coord_string){
    Coordinate coord;

    std::regex re(R"((\S+)\s(\S+))");
    std::smatch match;

    auto getCoordinate = [&](const std::string& coordStr, Coordinate& coord){
        std::regex re(R"((\d+)°(\d+)′([\d.]+)″([NSEOW]))");
        std::smatch match;

        if (std::regex_search(coordStr, match, re)) {
            double degrees = std::stod(match[1].str());
            double minutes = std::stod(match[2].str());
            double seconds = std::stod(match[3].str());
            char direction = match[4].str()[0];

            double decimal = degrees + minutes / 60.0 + seconds / 3600.0;

            switch (direction)
            {
            case 'N':
                coord.latitutde = LATITUDE::NORTH;
                coord.latitude_degree = decimal;
                break;
            case 'E':
                coord.longitude = LONGITUDE::EAST;
                coord.longitude_degree = decimal;
                break;
            case 'O':
                coord.longitude = LONGITUDE::EAST;
                coord.longitude_degree = decimal;
                break;
            case 'S':
                coord.latitutde = LATITUDE::SOUTH;
                coord.latitude_degree = (-1) * decimal;
                break;
            case 'W':
                coord.longitude = LONGITUDE::WEST;
                coord.longitude_degree = (-1) * decimal;
                break;
            
            default:
                break;
            }
            return;
        } else {
            throw std::invalid_argument("Ungültiges Koordinatenformat: " + coordStr);
        }
    };

    if (std::regex_search(coord_string, match, re)) {

        getCoordinate(match[1].str(), coord);
        getCoordinate(match[2].str(), coord);
    }

    return coord;
}

std::string stringToUpper(std::string str){
    for (char &c : str) {
        c = std::toupper(static_cast<unsigned char>(c));
    }
    return str;
}

bool downloadAndExtractData(const std::string& save_directory, const std::string& long_lat_string, const std::string& continent_prefix, WRITE_OUTPUT wout = WRITE_OUTPUT::DISCARD){
    if(!std::filesystem::exists(save_directory)){
        return false;
    }

    std::string baseUrl = "https://data.hydrosheds.org/file/";
    std::string logFile = save_directory + "log.txt";

    std::string dir = long_lat_string + "_dir";
    std::string con = long_lat_string + "_con";
    std::string acc = stringToUpper(long_lat_string) + "_acc";

    auto makeCmd = [&](const std::string& type, const std::string& prefix, const std::string& name) {
        std::string url = baseUrl + "hydrosheds-v1-" + type + "/" + continent_prefix + "_" + type + "_3s/" + name + ".zip";
        std::string localZip = save_directory + name + ".zip";
        return std::vector<std::string>{
            "wget -P " + save_directory + " " + url,
            "unzip -o " + localZip + " -d " + save_directory,
            "rm " + localZip
        };
    };

    auto execCmd = [&](const std::string& cmd){
        std::string cmd_;
        if (wout == WRITE_OUTPUT::DISCARD){
            cmd_ = cmd + " >> /dev/null 2>&1";
        }
        else if (wout == WRITE_OUTPUT::LOGFILE){
            cmd_ = cmd + " >> " + logFile + " 2>&1";
        }
        else{
            cmd_ = cmd;
        }
        system(cmd_.c_str());
        progressCount++;
        showProgressBar();
    };

    std::vector<std::string> flowDirCmds = makeCmd("dir", continent_prefix, dir);
    std::vector<std::string> conDemCmds = makeCmd("con", continent_prefix, con);
    std::vector<std::string> flowAccCmds = makeCmd("acc", continent_prefix, acc);

    std::vector<std::string> cmds;
    cmds.reserve(flowDirCmds.size() + conDemCmds.size() + flowAccCmds.size());

    cmds.insert(cmds.end(), flowDirCmds.begin(), flowDirCmds.end());
    cmds.insert(cmds.end(), conDemCmds.begin(), conDemCmds.end());
    cmds.insert(cmds.end(), flowAccCmds.begin(), flowAccCmds.end());
    if(wout == WRITE_OUTPUT::LOGFILE){
        system(("touch " + logFile).c_str());
    }

    for (std::string& cmd : cmds){
        execCmd(cmd);
    }

    return true;
}

bool getGeoData(const std::string& data_dir, const std::string& left_down_str, const std::string& right_up_str, WRITE_OUTPUT wout, bool keep_doc){
    Coordinate left_down = stringToCoordinate(left_down_str);
    Coordinate right_up = stringToCoordinate(right_up_str);

    int min_latitude  = static_cast<int>(std::floor(left_down.latitude_degree  / 10.0) * 10);
    int max_latitude  = static_cast<int>(std::ceil (right_up.latitude_degree   / 10.0) * 10);
    int min_longitude = static_cast<int>(std::floor(left_down.longitude_degree / 10.0) * 10);
    int max_longitude = static_cast<int>(std::ceil (right_up.longitude_degree  / 10.0) * 10);

    if(max_latitude < min_latitude || max_longitude < min_longitude){
        std::cout << "Requested has either negative height or width" << std::endl;
        return false;
    }

    total = ((max_latitude - min_latitude)/10) * ((max_longitude - min_longitude)/10) * (3 * 3 - 1);
    showProgressBar();

    auto format_coord = [](int value, char pos, char neg, int width) {
        char dir = (value >= 0) ? pos : neg;
        int abs_val = std::abs(value);
        std::ostringstream oss;
        oss << dir << std::setw(width) << std::setfill('0') << abs_val;
        return oss.str();
    };

    std::string save_dir = data_dir + format_coord(min_latitude, 'n', 's', 2) + format_coord(min_longitude, 'e', 'w', 3) + "_" + format_coord(max_latitude, 'n', 's', 2) + format_coord(max_longitude, 'e', 'w', 3) + "/";

    if(!std::filesystem::exists(save_dir)){
        system(("mkdir " + save_dir).c_str());
        system("rm -f log.txt");
    }
    else{
        system(("rm -r " + save_dir + "/*").c_str());
    }


    for (int latitude = min_latitude; latitude <= max_latitude; latitude = latitude + 10){
        for (int longitude = min_longitude; longitude <= max_longitude; longitude = longitude + 10){
            std::string downloadString;
            downloadString += format_coord(latitude,  'n', 's', 2);
            downloadString += format_coord(longitude, 'e', 'w', 3);
            
            std::pair<int, int> tile = {latitude, longitude};
            if(tileToContinentPrefix.count(tile)){
                downloadAndExtractData(save_dir, downloadString, tileToContinentPrefix[tile], wout);
            }
        }
    }

    if(!keep_doc){
        system(std::string("rm -f " + save_dir + "HydroSHEDS_TechDoc*.pdf").c_str());
    }

    return true;
}