#pragma once
#include <string>
#include <vector>

namespace Cel {

/**
 *
 */
class Console
{
  public:
    struct ConfigVar
    {
        std::string name;
        // Type corresponds to the field type in ImGui
        bool type;
    };

    /**
     * Creates a new window and header (if they don't exist) and adds the config
     * variables
     * @param name
     * @param header
     * @param vars
     */
    void add_config_vars(const std::string& name,
                         const std::string& header,
                         std::vector<ConfigVar> vars);

    /**
     * Creates a new window and header (if they don't exist) and adds the config
     * variable
     * @param name
     * @param header
     * @param var
     */
    void add_config_var(const std::string& name,
                        const std::string& header,
                        ConfigVar var);

  private:
    struct Heading
    {
        std::string name;
        std::vector<ConfigVar> vars;
    };

    struct Window
    {
        std::string name;
        std::vector<Heading> headings;
    };

    struct Config
    {
        std::string name;
        std::vector<ConfigVar> vars;
    };

    std::vector<Window> windows;
};

}