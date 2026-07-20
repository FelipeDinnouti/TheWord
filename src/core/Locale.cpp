#include "Locale.h"
#include <unordered_map>
#include <string_view>

namespace theword::core {
namespace {

const std::unordered_map<std::string_view, const char*>& GetTable() {
    static const std::unordered_map<std::string_view, const char*> table = {
        {"Reader", "Leitor"},
        {"Menu", "Menu"},
        {"Books", "Livros"},
        {"Settings", "Configura\u00e7\u00f5es"},
        {"Highlights", "Destaques"},
        {"Credits", "Cr\u00e9ditos"},
        {"Font:", "Fonte:"},
        {"Source:", "Origem:"},
        {"Color:", "Cor:"},
        {"Version:", "Vers\u00e3o:"},
        {"Theme:", "Tema:"},
        {"Light", "Claro"},
        {"Dark", "Escuro"},
        {"Back", "Voltar"},
            {"Del", "X"},
        {"Unknown", "Desconhecido"},
        {"(highlighted text)", "(texto destacado)"},
        {"(tap to filter)", "(toque para filtrar)"},
        {"No highlights yet.\nSelect text in the Reader to create one.",
         "Nenhum destaque ainda.\nSelecione um texto no Leitor para criar um."},
        {"No highlights of this color.", "Nenhum destaque desta cor."},
        {"Built with Raylib & C++17", "Constru\u00eddo com Raylib & C++17"},
        {"Data: USFM (offline) + YouVersion API (online)",
         "Dados: USFM (offline) + YouVersion API (online)"},
        {"Press Escape or tap outside to close",
         "Pressione Esc ou toque fora para fechar"},
        {"Loading...", "Carregando..."},
        {"Yellow", "Amarelo"},
        {"Pink", "Rosa"},
        {"Green", "Verde"},
        {"Blue", "Azul"},
        {"Orange", "Laranja"},
        {"--- POINT ---", "--- PONTO ---"},
        {"body", "corpo"},
        {"heading", "t\u00edtulo"},
        {"large", "grande"},
        {"small", "pequeno"},
        {"body bold", "corpo negrito"},
        {"--- BOLD (POINT) ---", "--- NEGRITO (PONTO) ---"},
        {"Font Diagnostic", "Diagn\u00f3stico de Fonte"},
        {"ESC to close", "ESC para fechar"},
    };
    return table;
}

} // namespace

const char* Locale::Get(const char* key) {
    auto& table = GetTable();
    auto it = table.find(key);
    if (it != table.end()) return it->second;
    return key;
}

} // namespace theword::core
