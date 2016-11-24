
local template = require "resty.template"
template.print = print

-- Developer option
template.cache = {}

function render()
    local layout = template.new(INCLUDEDIR .. "templates/base.html")
    layout.title = "Index"
    layout.active_page = "index"
    layout.view = template.compile(INCLUDEDIR .. "templates/index.html")

    layout:render()
end
