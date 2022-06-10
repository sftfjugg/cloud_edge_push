-- import the zzlib library
local zzlib = require("zzlib")
local file = assert(io.open("M-20200118-GE-21cls.zip", "rb"))
print("open zip")

--local output = zzlib.gunzipf("COPYING.gz")
--print("outPUt")
--local file_w = assert(io.open("COPYING2", "w+"))
--print("open for write")
--file_w:write(output)
--print("write")
--
local input = file:read("*all")

print("read zip")

local file_list = zzlib.filelist(input)
for _, v in ipairs(file_list) do
    print(v)
end
local output = zzlib.unzip(input, "M-20200118-GE-21cls.weight")
--local output = zzlib.inflate(input)

--local output = zzlib.gunzip(file)
print("unzip")

local file_w = assert(io.open("M-20200118-GE-21cls.weight", "wb+"))
print("open for write")
file_w:write(output)
print("write")
file_w:close()
file:close()
