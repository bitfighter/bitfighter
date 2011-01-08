# Author: Daniel Harple
#
# Purpose:
#   Replace tags and move *.in files

require 'yaml'
require 'fileutils'

# :file is the file we would like to process
# :tags is a string or array of tags we would like to replace. ex: @version@
# :in moves the file from :file to :file minus the ".in" extension
files = [
  {
    :file => "/src/macosx/config_common.h.in",
    :tags => "version",
    :in   => true
  },
  {
    :file => "/src/macosx/English.lproj/InfoPlist.strings.in",
    :tags => ["version","year"],
    :in   => true
  },
  {
    :file => "/config/aiplayers.cfg.in",
    :in   => true
  },
  {
    :file => "/language/languages.txt.in",
    :tags => "progtitle",
    :in   => true
  }
]

module BuildTools
  class ProcessFiles
    attr_reader :build_information
    
    TOP_LEVEL = ".."
    MAJOR_VERSION_FILE = "#{TOP_LEVEL}/major_version"
    MINOR_VERSION_FILE = "#{TOP_LEVEL}/minor_version"
    BUILD_INFORMATION_FILE  = "#{TOP_LEVEL}/macosx_build_information.yml"
    
    def initialize(files)
      @files = []               #! Files, and what actions we need to preform on them
      @build_information = {}   #! Generated build information, such as version and kind

      @build_information[:progtitle] = "Armagetron Advanced"
      
      # Append TOP_LEVEL to filenames
      files.each do |info|
        info[:file] = TOP_LEVEL + info[:file]
        @files << info
      end
        
      yield self if block_given?
    end
    
    # Helper method to generate everything we need to know about this build
    def generate_build_information
      self.determine_source_kind
      self.generate_version
      self.generate_year
      self.serialize_build_information
    end
    
    def generate_version
      game_version =
        if @build_information[:kind] == "CVS"
          %x("#{TOP_LEVEL}/batch/make/version" "#{TOP_LEVEL}").chomp
        else
          File.read("#{TOP_LEVEL}/src/macosx/config_common.h.in").scan(/#define VERSION "(.*)"/)[0][0]
        end
      @build_information[:version] = game_version
    end
    
    
    # Delete this file if you would like to generate new information. Also be sure to put the tags back in the files ( @version@, etc... )
    def generated_build_information_already?
      File.exists?(BUILD_INFORMATION_FILE)
    end
    
    def generate_year
      @build_information[:year] = Time.now.strftime("%Y")
    end
    
    # Determine if the source is checked out from CVS or it is a Release source
    def determine_source_kind
      if FileTest.directory?("#{TOP_LEVEL}/.svn") || FileTest.directory?("#{TOP_LEVEL}/.bzr")
        @build_information[:kind] = "CVS"
      else
        @build_information[:kind] = "Release"
      end
    end
    
    # Save the generated build information.
    def serialize_build_information
      open(BUILD_INFORMATION_FILE,'w') { |f| f.print(YAML.dump(@build_information)) }
    end
    
    # Deserialize the generated build information. This assumes +BUILD_INFORMATION_FILE+ exists.
    def deserialize_build_information
      @build_information = YAML.load_file(BUILD_INFORMATION_FILE)
    end
        
    # Replace a tag, ex. @tag@, with replace_with in a given file
    def replace_tag(file, tag, replace_with)
      replace_with = "\"#{replace_with}\"" if file =~ /config_common.h/ && tag == "version"
      
      open(file,'r+') do |f|
        text = f.read
        
        # Only mess with the file if we modified it (so we do not have to recompile the whole project)
        if text.gsub!(/\@#{tag}\@/, replace_with.to_s)
          f.pos = 0
          f.print(text)
          f.truncate(f.pos)
        end
      end
    end
    
    # Copy the .in file to +new_file+, and process +new_file+
    def process_in_file(file)
      new_name = file[0..-4]
      FileUtils.cp(file, new_name)
      return new_name
    end
    
    # Do what we need to do to each file
    def process_files
      @files.each do |file|
        puts "Processing #{file[:file]}"
        file[:file] = self.process_in_file(file[:file]) if file.has_key?(:in) and file[:in]
        file[:tags].each { |tag| self.replace_tag(file[:file], tag, @build_information[tag.to_sym]) } if file.has_key?(:tags)
      end      
    end
  end
end

if __FILE__ == $0
  # Delete the generated build information file to redo all the processing
  BuildTools::ProcessFiles.new(files) do |process|
    unless process.generated_build_information_already?
      puts "#{$0} Starting..."
      process.generate_build_information
      process.process_files
    end
  end
end
