require 'zip/zip'
require 'fileutils'
require 'win32api'
require 'pp'

$ignore  = [ '.git' ]
$bindir = '../bin/i386-pc-win32/Unicode_Release'

def zipfiles(zipfile, filespec, dest, ignore=[])

  spec = filespec
  spec = "#{filespec}/**/**" if File.directory?(filespec)

  Dir[spec].reject{ |f| f1=f.sub(dest+'/',''); ignore.count{|ig| File.fnmatch(ig,f1)} > 0 || f == zipfile || File.directory?(f) }.each do |file|
    
    d = dest
    d = File.join(dest, file.sub(filespec+'/','')) if File.directory?(filespec)
    puts "Adding:    #{d}"
    zipfile.add(d, file)

  end
end

def version(file)

  s = ""
  v = '0.0.0'

  # get the amount of space required to read in the version info
  vsize = Win32API.new('version.dll', 'GetFileVersionInfoSize', ['P', 'P'], 'L').call(file, s)

  if vsize > 0
    result = ' ' * vsize

    # read the version
    Win32API.new('version.dll', 'GetFileVersionInfo', ['P', 'L', 'L', 'P'], 'L').call(file, 0, vsize, result)

    rstring = result.unpack('v*').map{ |s| s.chr if s < 256} * ''

    r = /FileVersion\D+([\d\.]+)\000/.match(rstring)
    puts "Packaging: #{file} - version #{r[1]}" if r

    r = /ProductVersion\D+([\d|\.]+)/.match(rstring)
    v = r[1] if r

  end

  return v  
end

def main()

  # get the fileversion from exe resource section
  ver = version(File.join($bindir, 'HexEdit.exe'))

  begin
    Dir.mkdir('out')
  rescue
  end
  zipname = "out/hexedit-#{ver}.zip"

  puts "Building:  #{zipname}"

  # delete any existing zip
  FileUtils.rm zipname, :force=>true

  # build the zip!
  Zip::ZipFile.open(zipname, Zip::ZipFile::CREATE) do |zipfile|

    zipfiles(zipfile, '../README.TXT',                   'HexEdit/README.TXT',  $ignore)
    zipfiles(zipfile, '../LICENCE.TXT',                  'HexEdit/LICENCE.TXT', $ignore)
    zipfiles(zipfile, File.join($bindir, 'HexEdit.exe'), 'HexEdit/HexEdit.exe', $ignore)
    zipfiles(zipfile, File.join($bindir, 'typelib'),     'HexEdit/typelib',     $ignore)

  end

  len = File.size(zipname)
  puts "Done:      #{len} bytes"
end

main()

