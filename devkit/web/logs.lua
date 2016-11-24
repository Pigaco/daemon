local template = require "resty.template"
template.print = print

-- Developer option
template.cache = {}

function render()
    local layout = template.new(INCLUDEDIR .. "templates/base.html")
    layout.title = "Logs"
    layout.active_page = "logs"
    layout.view = template.compile(INCLUDEDIR .. "templates/logs.html")

    layout:render()
end
