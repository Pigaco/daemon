local template = require "resty.template"
template.print = print

-- Developer option
template.cache = {}

function render()
    local layout = template.new(INCLUDEDIR .. "templates/base.html")
    layout.title = "Devkit"
    layout.active_page = "devkit"
    layout.view = template.compile(INCLUDEDIR .. "templates/devkit.html")

    layout:render()
end

