# add muggle library
function(addMuggleLib name is_pure_c export_macro folder_name link_targets dependencies)

	# files
	file(GLOB ${name}_h ${muggle_source_dir}/muggle/${folder_name}/*.h)
	file(GLOB ${name}_c ${muggle_source_dir}/muggle/${folder_name}/*.c)
	file(GLOB ${name}_cpp ${muggle_source_dir}/muggle/${folder_name}/*.cpp)
	
	if (${is_pure_c})
		file(GLOB ${name}_src 
			${${name}_c}
		)
	else()
		file(GLOB ${name}_src 
			${${name}_c}
			${${name}_cpp}
		)
	endif()
	
	# add lib
	if (WIN32)
		add_library(${name} ${MUGGLE_LIB_TYPE}
			${${name}_h}
			${${name}_src}
		)
	else()
		add_library(${name} ${MUGGLE_LIB_TYPE}
			${${name}_src}
		)
	endif()

	# property
	if (${is_pure_c})
		set_target_properties(${name} PROPERTIES 
			LINKER_LANGUAGE C
		)
	endif()
	
	# link
	if (NOT link_targets STREQUAL "")
		target_link_libraries(${name} 
			${link_targets}
		)
	endif()

	# dependencies
	if (NOT dependencies STREQUAL "")
		add_dependencies(${name}
			${dependencies}
		)
	endif()

	# windows dll export and import
	if(MSVC AND ${MUGGLE_BUILD_SHARED_LIB})
	target_compile_definitions(${name}
		PUBLIC MUGGLE_USE_DLLS
		PRIVATE ${export_macro} 
	)
	endif()

	# install
	install(FILES ${${name}_h} DESTINATION ${MUGGLE_INSTALL_INCLUDE_PATH}/${folder_name})
	install(TARGETS ${name}
		RUNTIME DESTINATION ${MUGGLE_INSTALL_RUNTIME_PATH}
		LIBRARY DESTINATION ${MUGGLE_INSTALL_LIBRARY_PATH}
		ARCHIVE DESTINATION ${MUGGLE_INSTALL_ARCHIVE_PATH}
	)

endfunction(addMuggleLib)

# add simple project
function(addProject name folder_name dependencies type)
	file(GLOB ${name}_H ${folder_name}/*.h)
	file(GLOB ${name}_C ${folder_name}/*.c)
	file(GLOB ${name}_CPP ${folder_name}/*.cpp)
	
	if (type STREQUAL "EXE")
		if (WIN32)
			add_executable(${name}
				${${name}_H}
				${${name}_C}
				${${name}_CPP}
			)
		else()
			add_executable(${name}
				${${name}_C}
				${${name}_CPP}
			)
		endif()
	elseif(type STREQUAL "DLL")
		if (WIN32)
			add_library(${name} SHARED
				${${name}_H}
				${${name}_C}
				${${name}_CPP}
			)
			# windows dll export and import
			target_compile_definitions(${name}
				PUBLIC ${name}_USE_DLL
				PRIVATE ${name}_EXPORTS 
			)
		else()
			add_library(${name} SHARED
				${${name}_C}
				${${name}_CPP}
			)
		endif()
	else()
		if (WIN32)
			add_library(${name}
				${${name}_H}
				${${name}_C}
				${${name}_CPP}
			)
		else()
			add_library(${name}
				${${name}_C}
				${${name}_CPP}
			)
		endif()
	endif()
	
	if (NOT dependencies STREQUAL "")
		target_link_libraries(${name} 
			${dependencies}
		)
		add_dependencies(${name}
			${dependencies}
		)
	endif()
endfunction(addProject) 