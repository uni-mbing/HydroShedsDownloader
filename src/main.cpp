#include <iostream>
#include <CLI11.hpp>
#include <ParseCoordinates.hpp>

int main(int argc, char** argv){
    CLI::App app{"App description"};
    argv = app.ensure_utf8(argv);

    std::string coord_lower_left = "40°18′55.70″N 8°13′4.32″O";
    std::string coord_upper_right = "60°14′06.70″N 21°0′55.60″O";
    std::string save_directory = "/home/max/Documents/Code/Uni/Advanced_Software_Internship/Automate_Downloads/data/";
    bool verbose_output = false;
    bool write_log = false;
    bool keep_doc = false;

    app.add_option("-l,--lower-left", coord_lower_left, "String for the lower left coordinate");
    app.add_option("-r,--upper-right", coord_upper_right, "String for the upper right coordinate");
    app.add_option("-s,--save-dir", save_directory, "Directory to save the downloaded data to");
    app.add_flag("-v,--verbose", verbose_output, "Writes output to console or discards it");
    app.add_flag("-f, --log-file", write_log, "Writes output to logfile");
    app.add_flag("-d, --docs", keep_doc, "Keeps HydroSheds documentation");

    CLI11_PARSE(app, argc, argv);

    return downloadGeoData(save_directory, coord_lower_left, coord_upper_right, (verbose_output) ? WRITE_OUTPUT::CONSOLE : ((write_log) ? WRITE_OUTPUT::LOGFILE : WRITE_OUTPUT::DISCARD), keep_doc);
}