#include "swtor_parser_enhanced.h"
#include "swtor_parser_helpers.h"
#include <iostream>
#include <vector>

/**
 * Example usage of enhanced SWTOR parser with AreaEntered and DisciplineChanged support
 */

void demonstrate_area_entered() {
    std::cout << "=== AreaEntered Detection Example ===" << std::endl;
    
    // Example lines
    std::vector<std::string> test_lines = {
        "[23:54:32.862] [@Pugonefive#689739618206834|(-5.43,-75.83,39.02,66.32)|(1/183817)] [] [] [AreaEntered {836045448953664}: Nar Shaddaa {137438987989}] (he3001) <v7.0.0b>",
        "[21:37:53.211] [@Pugzeroeight#688448512393486|(-384.05,169.28,0.17,-91.58)|(421327/437086)] [] [] [AreaEntered {836045448953664}: Dxun - The CI-004 Facility {833571547775792} 8 Player Master {836045448953655}] (he3001) <v7.0.0b>"
    };
    
    for (const auto& line_str : test_lines) {
        swtor::CombatLine line;
        auto status = swtor::parse_combat_line(line_str, line);
        
        if (status == swtor::ParseStatus::Ok) {
            // Method 1: Using EventTag comparison
            if (line == swtor::Events::AreaEntered) {
                std::cout << "✓ Detected AreaEntered (using EventTag)" << std::endl;
            }
            
            // Method 2: Using EventPred comparison
            if (line == swtor::Evt::AreaEntered) {
                std::cout << "✓ Detected AreaEntered (using EventPred)" << std::endl;
                
                // Access area-specific data
                std::cout << "  Area: " << line.area_entered.area.name 
                         << " (ID: " << line.area_entered.area.id << ")" << std::endl;
                
                if (line.area_entered.has_difficulty) {
                    std::cout << "  Difficulty: " << line.area_entered.difficulty.name
                             << " (ID: " << line.area_entered.difficulty.id << ")" << std::endl;
                }
                
                if (!line.area_entered.version.empty()) {
                    std::cout << "  Version: " << line.area_entered.version << std::endl;
                }
                
                if (!line.area_entered.raw_value.empty()) {
                    std::cout << "  Raw Value: " << line.area_entered.raw_value << std::endl;
                }
            }
            
            // Method 3: Using helper function
            if (swtor::is_area_entered(line)) {
                std::cout << "✓ Detected AreaEntered (using helper function)" << std::endl;
                auto area = swtor::get_area_name(line);
                auto difficulty = swtor::get_difficulty_name(line);
                std::cout << "  Quick access - Area: " << area;
                if (!difficulty.empty()) {
                    std::cout << ", Difficulty: " << difficulty;
                }
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
    }
}

void demonstrate_discipline_changed() {
    std::cout << "=== DisciplineChanged Detection Example ===" << std::endl;
    
    std::string test_line = 
        "[21:31:26.309] [@Pugzeroeight#688448512393486|(24.16,27.19,9.63,-93.38)|(1/226209)] [] [] "
        "[DisciplineChanged {836045448953665}: Operative {16140905232405801950}/Lethality {2031339142381593}]";
    
    swtor::CombatLine line;
    auto status = swtor::parse_combat_line(test_line, line);
    
    if (status == swtor::ParseStatus::Ok) {
        // Method 1: Using EventTag
        if (line == swtor::Events::DisciplineChanged) {
            std::cout << "✓ Detected DisciplineChanged (using EventTag)" << std::endl;
        }
        
        // Method 2: Using EventPred
        if (line == swtor::Evt::DisciplineChanged) {
            std::cout << "✓ Detected DisciplineChanged (using EventPred)" << std::endl;
            
            // Access discipline-specific data
            std::cout << "  Combat Class: " << line.discipline_changed.combat_class.name
                     << " (ID: " << line.discipline_changed.combat_class.id << ")" << std::endl;
            std::cout << "  Discipline: " << line.discipline_changed.discipline.name
                     << " (ID: " << line.discipline_changed.discipline.id << ")" << std::endl;
            
            // Access enum values
            std::cout << "  Class enum: " 
                     << swtor::combat_class_name(line.discipline_changed.combat_class_enum) 
                     << std::endl;
            std::cout << "  Discipline enum: " 
                     << swtor::discipline_name(line.discipline_changed.discipline_enum) 
                     << std::endl;
        }
        
        // Method 3: Using helper function
        if (swtor::is_discipline_changed(line)) {
            std::cout << "✓ Detected DisciplineChanged (using helper function)" << std::endl;
        }
    }
    std::cout << std::endl;
}

void demonstrate_filtering() {
    std::cout << "=== Event Filtering Examples ===" << std::endl;
    
    std::vector<std::string> mixed_lines = {
        "[01:28:40.284] [@Pugzeroone#688438700531589|(246.10,-141.57,-232.11,-88.18)|(450254/450254)] "
        "[Lord Kanoth {4494876548792320}:11760001999148|(260.00,-144.00,-232.06,116.61)|(43894876/43909316)] "
        "[Ion Cell {3963919806758912}] [ApplyEffect {836045448945477}: Shocked {3963919806759210}] "
        "(937 ~936 energy {836045448940874}) <2811>",
        
        "[23:54:32.862] [@Pugonefive#689739618206834|(-5.43,-75.83,39.02,66.32)|(1/183817)] [] [] "
        "[AreaEntered {836045448953664}: Nar Shaddaa {137438987989}] (he3001) <v7.0.0b>",
        
        "[21:31:26.309] [@Pugzeroeight#688448512393486|(24.16,27.19,9.63,-93.38)|(1/226209)] [] [] "
        "[DisciplineChanged {836045448953665}: Operative {16140905232405801950}/Lethality {2031339142381593}]"
    };
    
    int area_count = 0;
    int discipline_count = 0;
    int damage_count = 0;
    
    for (const auto& line_str : mixed_lines) {
        swtor::CombatLine line;
        if (swtor::parse_combat_line(line_str, line) == swtor::ParseStatus::Ok) {
            // Count different event types using simple equality checks
            if (line == swtor::Evt::AreaEntered) {
                area_count++;
            } else if (line == swtor::Evt::DisciplineChanged) {
                discipline_count++;
            } else if (line == swtor::Evt::Damage) {
                damage_count++;
            }
        }
    }
    
    std::cout << "Summary:" << std::endl;
    std::cout << "  AreaEntered events: " << area_count << std::endl;
    std::cout << "  DisciplineChanged events: " << discipline_count << std::endl;
    std::cout << "  Damage events: " << damage_count << std::endl;
    std::cout << std::endl;
}

void demonstrate_combined_predicates() {
    std::cout << "=== Combined Predicate Examples ===" << std::endl;
    
    // Create a predicate that matches AreaEntered OR DisciplineChanged
    auto special_events = swtor::Evt::AreaEntered | swtor::Evt::DisciplineChanged;
    
    std::vector<std::string> test_lines = {
        "[01:28:40.284] [@Pugzeroone#688438700531589|(...)|(...)] [Target|(...)|(...)] "
        "[Ability] [ApplyEffect {836045448945477}: Damage {836045448945501}] (1000)",
        
        "[23:54:32.862] [@Pugonefive#689739618206834|(...)|(...)] [] [] "
        "[AreaEntered {836045448953664}: Nar Shaddaa {137438987989}] (he3001) <v7.0.0b>",
        
        "[21:31:26.309] [@Pugzeroeight#688448512393486|(...)|(...)] [] [] "
        "[DisciplineChanged {836045448953665}: Operative {16140905232405801950}/Lethality {2031339142381593}]"
    };
    
    for (const auto& line_str : test_lines) {
        swtor::CombatLine line;
        if (swtor::parse_combat_line(line_str, line) == swtor::ParseStatus::Ok) {
            if (line == special_events) {
                std::cout << "✓ Matched special event (AreaEntered or DisciplineChanged)" << std::endl;
                if (line == swtor::Evt::AreaEntered) {
                    std::cout << "  → It's an AreaEntered: " << line.area_entered.area.name << std::endl;
                } else if (line == swtor::Evt::DisciplineChanged) {
                    std::cout << "  → It's a DisciplineChanged: " 
                             << line.discipline_changed.combat_class.name << "/"
                             << line.discipline_changed.discipline.name << std::endl;
                }
            } else {
                std::cout << "  (Regular event, not special)" << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "SWTOR Enhanced Parser Demo" << std::endl;
    std::cout << "==========================" << std::endl;
    std::cout << std::endl;
    
    demonstrate_area_entered();
    demonstrate_discipline_changed();
    demonstrate_filtering();
    demonstrate_combined_predicates();
    
    std::cout << "=== All Examples Complete ===" << std::endl;
    
    return 0;
}
