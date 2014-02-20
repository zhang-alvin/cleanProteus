.PHONY: all check clean distclean doc install profile

all: install

#We use environment variables from the invoking process if they have been set,
#otherwise we try our best to determine them automatically.

PROTEUS ?= $(shell pwd)
VER_CMD = git log -1 --pretty="%H"
# shell hack for now to automatically detect Garnet front-end nodes
PROTEUS_ARCH ?= $(shell [[ $$(hostname) = garnet* ]] && echo "garnet.gnu" || python -c "import sys; print sys.platform")
PROTEUS_PREFIX ?= ${PROTEUS}/${PROTEUS_ARCH}
PROTEUS_PYTHON ?= ${PROTEUS_PREFIX}/bin/python
PROTEUS_VERSION := $(shell ${VER_CMD})

ifeq ($(PROTEUS_ARCH), darwin)
PLATFORM_ENV = MACOSX_DEPLOYMENT_TARGET=$(shell sw_vers -productVersion | sed "s/\(10.[0-9]\).*/\1/")
endif

ifeq ($(PROTEUS_ARCH), Cygwin)
BOOTSTRAP = cygwin_bootstrap.done
endif

# The choice for default Fortran compiler needs to be overridden on the Garnet system
ifeq ($(PROTEUS_ARCH), garnet.gnu)
FC=ftn 
F77=ftn 
F90=ftn
endif 


PROTEUS_ENV ?= PATH="${PROTEUS_PREFIX}/bin:${PATH}" \
	PYTHONPATH=${PROTEUS_PREFIX}/lib/python2.7/site-packages \
	PROTEUS_PREFIX=${PROTEUS_PREFIX} \
	PROTEUS=${PROTEUS} \
	${PLATFORM_ENV}

clean:
	-PROTEUS_PREFIX=${PROTEUS_PREFIX} ${PROTEUS_PYTHON} setuppyx.py clean
	-PROTEUS_PREFIX=${PROTEUS_PREFIX} ${PROTEUS_PYTHON} setupf.py clean
	-PROTEUS_PREFIX=${PROTEUS_PREFIX} ${PROTEUS_PYTHON} setuppetsc.py clean
	-PROTEUS_PREFIX=${PROTEUS_PREFIX} ${PROTEUS_PYTHON} setup.py clean

distclean: clean
	-rm -f config.py configure.done stack.done
	-rm -rf ${PROTEUS_PREFIX}
	-rm -rf build src/*.pyc src/*.so src/*.a

hashdist: 
	@echo "No hashdist found.  Cloning hashdist from GitHub"
	git clone https://github.com/hashdist/hashdist.git

stack: 
	@echo "No stack found.  Cloning stack from GitHub"
	git clone https://github.com/hashdist/hashstack.git stack

cygwin_bootstrap.done: stack/scripts/setup_cygstack.py stack/scripts/cygstack.txt
	python hashstack/scripts/setup_cygstack.py hashstack/scripts/cygstack.txt
        touch cygwin_bootstrap.done

profile: ${PROTEUS_PREFIX}/artifact.json

# A hashstack profile will be rebuilt if Make detects any files in the stack 
# directory newer than the profile artifact file.
${PROTEUS_PREFIX}/artifact.json: stack hashdist $(shell find stack -type f) ${BOOTSTRAP}
	@echo "************************"
	@echo "Building dependencies..."
	@echo "************************"

	cp stack/examples/proteus.${PROTEUS_ARCH}.yaml stack/default.yaml
	cd stack && ${PROTEUS}/hashdist/bin/hit develop -k error -f ${PROTEUS_PREFIX}
        # workaround hack on Cygwin for hashdist launcher to work correctly
	-cp ${PROTEUS}/${PROTEUS_ARCH}/bin/python2.7.exe.link ${PROTEUS}/${PROTEUS_ARCH}/bin/python2.7.link

	@echo "************************"
	@echo "Dependency build complete"
	@echo "************************"


#config.py file should be newer than proteusConfig/config.py.$PROTEUS_ARCH
config.py: proteusConfig/config.py.${PROTEUS_ARCH}
	@echo "************************"
	@echo "Configuring..."
	@echo "************************"
	@echo "Copying proteusConfig/config.py.$PROTEUS_ARCH to ./config.py"
	@cp proteusConfig/config.py.${PROTEUS_ARCH} config.py
	@echo "************************"
	@echo "Configure complete"
	@echo "************************"


# Proteus install should be triggered by an out-of-date hashstack profile, source tree, or modified setup files.
install: profile config.py $(shell find src -type f) $(wildcard *.py)
	@echo "************************"
	@echo "Installing..."
	@echo "************************"
	${PROTEUS_ENV} ${PROTEUS_PYTHON} setuppyx.py install
	@echo "************************"
	@echo "done installing cython extension modules"
	@echo "************************"
	${PROTEUS_ENV} ${PROTEUS_PYTHON} setupf.py install
	@echo "************************"
	@echo "done installing f2py extension modules"
	@echo "************************"
	${PROTEUS_ENV} ${PROTEUS_PYTHON} setuppetsc.py build --petsc-dir=${PROTEUS_PREFIX} --petsc-arch='' install
	@echo "************************"
	@echo "done installing petsc-based extension modules"
	@echo "************************"
	${PROTEUS_ENV} ${PROTEUS_PYTHON} setup.py install
	@echo "************************"
	@echo "done installing standard extension modules"
	@echo "************************"
	@echo "Installation complete"
	@echo "************************"
	@echo "\n"
	@echo "Proteus was built using the following configuration:"
	@echo "Please include this information in all bug reports."
	@echo "+======================================================================================================+"
	@echo "PROTEUS          : ${PROTEUS}"
	@echo "PROTEUS_ARCH     : ${PROTEUS_ARCH}"
	@echo "PROTEUS_PREFIX   : ${PROTEUS_PREFIX}"
	@echo "PROTEUS_VERSION  : ${PROTEUS_VERSION}"
	@echo "HASHSTACK_VERSION: $$(cd hashdist; ${VER_CMD})"
	@echo "HASHDIST_VERSION : $$(cd stack; ${VER_CMD})"
	@echo "+======================================================================================================+"
	@echo "\n"
	@echo "You should now verify that the install succeeded by running:"
	@echo "\n"
	@echo "make check"
	@echo "\n"

check: install
	@echo "************************"
	@echo "Sanity environment check"
	@echo PROTEUS: ${PROTEUS}
	@echo PROTEUS_ARCH: ${PROTEUS_ARCH}
	@echo PROTEUS_PREFIX: ${PROTEUS_PREFIX}
	@echo PROTEUS_ENV: ${PROTEUS_ENV}

	@echo "************************"
	@echo "Hello world Check!"
	${PROTEUS_ENV} python -c "print 'hello world'"
	@echo "************************"
	@echo "Proteus Partition Test"
	${PROTEUS_ENV} python test/test_meshParitionFromTetgenFiles.py
	@echo "************************"

	@echo "************************"
	@echo "Parallel Proteus Partition Test"
	${PROTEUS_ENV} mpirun -np 4 python test/test_meshParitionFromTetgenFiles.py
	@echo "************************"

doc: install
	cd doc && ${PROTEUS_ENV} make html
