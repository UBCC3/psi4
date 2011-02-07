// Include Python's header file.
// Use "python-config --includes" to determine this location.

#define BOOST_FILESYSTEM_VERSION 3

//  As an example program, we don't want to use any deprecated features
#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
#  define BOOST_FILESYSTEM_NO_DEPRECATED
#endif
#ifndef BOOST_SYSTEM_NO_DEPRECATED
#  define BOOST_SYSTEM_NO_DEPRECATED
#endif

#include <cstdio>
#include <boost/algorithm/string.hpp>
#include <boost/python.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <psiconfig.h>
#include <sstream>
#include "script.h"
#include "liboptions/liboptions.h"
#include <libmints/molecule.h>
#include <libplugin/plugin.h>
#include <libparallel/parallel.h>
#include <map>

#include <psi4-dec.h>

using namespace psi;
//using namespace psi::functional;
using namespace boost;
using namespace boost::python;
using namespace std;

// Python helper wrappers
void export_benchmarks();
void export_blas_lapack();
void export_plugins();
void export_psio();
void export_chkpt();
void export_mints();
void export_functional();

// In export_plugins.cc
void py_psi_plugin_close_all();

extern std::map<std::string, plugin_info> plugins;

namespace opt      { psi::PsiReturnType optking(psi::Options &); }

namespace psi {
    namespace mints     { PsiReturnType mints(Options &); }
    namespace deriv     { PsiReturnType deriv(Options &); }
    namespace scf       { PsiReturnType scf(Options &, PyObject* pre, PyObject* post);   }
    namespace dfmp2     { PsiReturnType dfmp2(Options &); }
    namespace sapt      { PsiReturnType sapt(Options &);  }
    namespace dcft      { PsiReturnType dcft(Options &);  }

    namespace transqt   { PsiReturnType transqt(Options &);  }
    namespace transqt2  { PsiReturnType transqt2(Options &); }
    namespace ccsort    { PsiReturnType ccsort(Options&);    }
    namespace ccenergy  { PsiReturnType ccenergy(Options&);  }
    namespace cctriples { PsiReturnType cctriples(Options&); }
    namespace cchbar    { PsiReturnType cchbar(Options&);    }
    namespace cclambda  { PsiReturnType cclambda(Options&);  }
    namespace ccdensity { PsiReturnType ccdensity(Options&); }
    namespace oeprop    { PsiReturnType oeprop(Options&);    }

    extern int read_options(const std::string &name, Options & options, bool suppress_printing = false);
    extern FILE *outfile;
}

int py_psi_optking()
{
    return opt::optking(Process::environment.options);
}

int py_psi_mints()
{
    return mints::mints(Process::environment.options);
}

int py_psi_deriv()
{
    return deriv::deriv(Process::environment.options);
}

double py_psi_scf_callbacks(PyObject* precallback, PyObject* postcallback)
{
    if (scf::scf(Process::environment.options, precallback, postcallback) == Success) {
        return Process::environment.globals["CURRENT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_scf()
{
    return py_psi_scf_callbacks(Py_None, Py_None);
}

double py_psi_dcft()
{
    if (dcft::dcft(Process::environment.options) == Success) {
        return Process::environment.globals["CURRENT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_dfmp2()
{
    if (dfmp2::dfmp2(Process::environment.options) == Success) {
        return Process::environment.globals["CURRENT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_sapt()
{
    if (sapt::sapt(Process::environment.options) == Success) {
        return Process::environment.globals["SAPT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_transqt()
{
    transqt::transqt(Process::environment.options);
    return 0.0;
}

double py_psi_transqt2()
{
    transqt2::transqt2(Process::environment.options);
    return 0.0;
}

double py_psi_ccsort()
{
    ccsort::ccsort(Process::environment.options);
    return 0.0;
}

double py_psi_ccenergy()
{
    if (ccenergy::ccenergy(Process::environment.options) == Success) {
        return Process::environment.globals["CURRENT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_cctriples()
{
    if (cctriples::cctriples(Process::environment.options) == Success) {
        return Process::environment.globals["CURRENT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_cchbar()
{
    cchbar::cchbar(Process::environment.options);
    return 0.0;
}

double py_psi_cclambda()
{
    cclambda::cclambda(Process::environment.options);
    return 0.0;
}

double py_psi_ccdensity()
{
    ccdensity::ccdensity(Process::environment.options);
    return 0.0;
}

double py_psi_oeprop()
{
    oeprop::oeprop(Process::environment.options);
    return 0.0;
}

char const* py_psi_version()
{
    return PSI_VERSION;
}

void py_psi_clean()
{
    PSIOManager::shared_object()->psiclean();
}

void py_psi_set_default_options_for_module(std::string const & name)
{
    read_options(name, Process::environment.options, false);

    if (plugins.count(name)) {
        // Easy reference
        plugin_info& info = plugins[name];

        // Tell the plugin to load in its options into the current environment.
        info.read_options(info.name, Process::environment.options);
    }
}

void py_psi_print_options()
{
    Process::environment.options.print();
}

void py_psi_print_global_options()
{
    Process::environment.options.print_globals();
}

void py_psi_print_out(std::string s)
{
    fprintf(outfile,"%s",s.c_str());
}

bool py_psi_set_option_string(std::string const & name, std::string const & value)
{
    Process::environment.options.set_str(name, value);

    string nonconst_key = name;
    Data& data = Process::environment.options.use(nonconst_key);

    if (data.type() == "string") {
        Process::environment.options.set_str(name, value);
    } else if (data.type() == "boolean") {
        if (boost::to_upper_copy(value) == "TRUE" || boost::to_upper_copy(value) == "YES" || \
          boost::to_upper_copy(value) == "ON")
            Process::environment.options.set_int(name, true);
        else if (boost::to_upper_copy(value) == "FALSE" || boost::to_upper_copy(value) == "NO" || \
          boost::to_upper_copy(value) == "OFF")
            Process::environment.options.set_int(name, false);
        else
            throw std::domain_error("Required option type is boolean, no boolean specified");
    }
    return true;
}

bool py_psi_set_option_int(std::string const & name, int value)
{
    Process::environment.options.set_int(name, value);
    return true;
}

// Right now this can only handle arrays of integers.
// Unable to handle strings.
bool py_psi_set_option_array(std::string const & name, python::list values)
{
    size_t n = len(values);

    // Reset the array to a known state (empty).
    Process::environment.options[name].reset();

    for (size_t i=0; i < n; ++i) {
        Process::environment.options[name].add(extract<double>(values[i]));
    }

    return true;
}

bool py_psi_set_global_option_string(std::string const & name, std::string const & value)
{
    //Process::environment.options.set_global_str(name, value);

    string nonconst_key = name;
    Data& data = Process::environment.options.use(nonconst_key);

    if (data.type() == "string") {
        Process::environment.options.set_global_str(name, value);
    } else if (data.type() == "boolean") {
        if (boost::to_upper_copy(value) == "TRUE" || boost::to_upper_copy(value) == "YES" || \
          boost::to_upper_copy(value) == "ON")
            Process::environment.options.set_global_int(name, true);
        else if (boost::to_upper_copy(value) == "FALSE" || boost::to_upper_copy(value) == "NO" || \
          boost::to_upper_copy(value) == "OFF")
            Process::environment.options.set_global_int(name, false);
        else
            throw std::domain_error("Required option type is boolean, no boolean specified");
    }
    return true;
}

bool py_psi_set_global_option_int(std::string const & name, int value)
{
    Process::environment.options.set_global_int(name, value);
    return true;
}
// Right now this can only handle arrays of integers.
// Unable to handle strings.
bool py_psi_set_global_option_array(std::string const & name, python::list values)
{
    size_t n = len(values);

    // Reset the array to a known state (empty).
    // Process::environment.options[name].reset();
    Process::environment.options.get_global(name).reset();

    for (size_t i=0; i < n; ++i) {
        Process::environment.options.get_global(name).add(extract<double>(values[i]));
    }

    return true;
}

object py_psi_get_option(const string& key)
{
    string nonconst_key = key;
    Data& data = Process::environment.options.use(nonconst_key);

    if (data.type() == "string")
        return str(data.to_string());
    else if (data.type() == "boolean" || data.type() == "integer")
        return object(data.to_integer());
    else if (data.type() == "double")
        return object(data.to_double());

    return object();
}

object py_psi_get_global_option(const string& key)
{
    string nonconst_key = key;
    Data& data = Process::environment.options.use(nonconst_key);

    if (data.type() == "string")
        return str(data.to_string());
    else if (data.type() == "boolean" || data.type() == "integer")
        return object(data.to_integer());
    else if (data.type() == "double")
        return object(data.to_double());

    return object();
}

void py_psi_set_active_molecule(shared_ptr<Molecule> molecule)
{
    Process::environment.set_molecule(molecule);
}

boost::shared_ptr<Molecule> py_psi_get_active_molecule()
{
    return Process::environment.molecule();
}

double py_psi_get_variable(const std::string & key)
{
    string uppercase_key = key;
    transform(uppercase_key.begin(), uppercase_key.end(), uppercase_key.begin(), ::toupper);
    return Process::environment.globals[uppercase_key];
}

void py_psi_set_memory(unsigned long int mem)
{
    Process::environment.set_memory(mem);
    fprintf(outfile,"\n  Memory set to %7.3f %s by Python script.\n",(mem > 1000000000 ? mem/1.0E9 : mem/1.0E6), \
        (mem > 1000000000 ? "GiB" : "MiB" ));
}

unsigned long int py_psi_get_memory()
{
    return Process::environment.get_memory();
}

void py_psi_set_n_threads(int nthread)
{
    Process::environment.set_n_threads(nthread);
}

int py_psi_get_n_threads()
{
    return Process::environment.get_n_threads();
}

BOOST_PYTHON_MODULE(PsiMod)
{
    def("version", py_psi_version);
    def("clean", py_psi_clean);

    // Benchmarks
    export_benchmarks();

    // BLAS/LAPACK Static Wrappers
    export_blas_lapack();

    // Plugins
    export_plugins();

    // Options
    def("set_default_options_for_module", py_psi_set_default_options_for_module);
    def("set_active_molecule", py_psi_set_active_molecule);
    def("get_active_molecule", &py_psi_get_active_molecule);
    def("set_memory", py_psi_set_memory);
    def("get_memory", py_psi_get_memory);
    def("set_n_threads", &py_psi_set_n_threads);
    def("get_n_threads", &py_psi_get_n_threads);

    def("print_options", py_psi_print_options);
    def("print_global_options", py_psi_print_global_options);
    def("print_out", py_psi_print_out);

    def("set_option", py_psi_set_option_string);
    def("set_option", py_psi_set_option_int);
    def("set_option", py_psi_set_option_array);

    def("set_global_option", py_psi_set_global_option_string);
    def("set_global_option", py_psi_set_global_option_int);
    def("set_global_option", py_psi_set_global_option_array);

    def("get_option", py_psi_get_option);
    def("get_global_option", py_psi_get_global_option);

    def("get_variable", py_psi_get_variable);

    // modules
    def("mints", py_psi_mints);
    def("deriv", py_psi_deriv);

    typedef double (*scf_module_none)();
    typedef double (*scf_module_two)(PyObject*, PyObject*);

    def("scf", py_psi_scf_callbacks);
    def("scf", py_psi_scf);
    def("dcft", py_psi_dcft);
    def("dfmp2", py_psi_dfmp2);
    def("sapt", py_psi_sapt);
    def("optking", py_psi_optking);
    def("transqt", py_psi_transqt);
    def("transqt2", py_psi_transqt2);
    def("ccsort", py_psi_ccsort);
    def("ccenergy", py_psi_ccenergy);
    def("cctriples", py_psi_cctriples);
    def("cchbar", py_psi_cchbar);
    def("cclambda", py_psi_cclambda);
    def("ccdensity", py_psi_ccdensity);
    def("oeprop", py_psi_oeprop);

    // Define library classes
    export_psio();
    export_chkpt();
    export_mints();
    export_functional();

    /**
    class_<DFTensor, shared_ptr<DFTensor> >( "DFTensor", no_init).
        def("bootstrap", &DFTensor::bootstrap_DFTensor).
        staticmethod("booststrap").
        def("form_fitting_metric", &DFTensor::form_fitting_metric).
        def("form_cholesky_metric", &DFTensor::form_cholesky_metric).
        def("form_qr_metric", &DFTensor::form_qr_metric).
        def("finalize", &DFTensor::finalize).
        def("print_out", &DFTensor::print_python);
    **/

    typedef string (Process::Environment::*environmentStringFunction)(const string&);

    class_<Process::Environment>("Environment").
        def("__getitem__", environmentStringFunction(&Process::Environment::operator ()));
//        def("set", &Process::Environment::set);

    typedef string (Process::Arguments::*argumentsStringFunction)(int);

    class_<Process::Arguments>("Arguments").
        def("__getitem__", argumentsStringFunction(&Process::Arguments::operator ()));

    class_<Process>("Process").
        add_static_property("environment", &Process::environment);

}

Python::Python() : Script()
{

}

Python::~Python()
{

}

void Python::initialize()
{
}

void Python::finalize()
{
}

void Python::run(FILE *input)
{
    using namespace boost::python;
    char *s;
    if (input == NULL)
        return;

    // Setup globals options
    Process::environment.options.set_read_globals(true);
    read_options("", Process::environment.options, true);
    Process::environment.options.set_read_globals(false);

    if (!Py_IsInitialized()) {
        s = strdup("psi");
        // Py_InitializeEx(0) causes sig handlers to not be installed.
        Py_InitializeEx(0);
        #if PY_VERSION_HEX >= 0x03000000
        Py_SetProgramName(L"psi");
        #else
        Py_SetProgramName(s);
        #endif

        // Track down the location of PSI4's python script directory.
        std::string psiDataDirName = Process::environment("PSIDATADIR") + "/python";
        boost::filesystem::path bf_path;
        bf_path = boost::filesystem::system_complete(psiDataDirName);
        if(!boost::filesystem::is_directory(bf_path))
            throw PSIEXCEPTION("Unable to read the Python folder - check the PSIDATADIR environmental variable");

        // Add PSI library python path
        PyObject *path, *sysmod, *str;
        sysmod = PyImport_ImportModule("sys");
        path = PyObject_GetAttrString(sysmod, "path");
        str = PyString_FromString(psiDataDirName.c_str());
        PyList_Append(path, str);
        Py_DECREF(str);
        Py_DECREF(path);
        Py_DECREF(sysmod);
    }
    if (Py_IsInitialized()) {
        // Stupid way to read in entire file.
        char line[256];
        std::stringstream file;
        while(fgets(line, sizeof(line), input)) {
            file << line;
        }

        // Process the input file
        PyObject *input = PyImport_ImportModule("input");
        PyObject *function = PyObject_GetAttrString(input, "process_input");
        PyObject *pargs = Py_BuildValue("(s)", file.str().c_str());
        PyObject *ret = PyEval_CallObject(function, pargs);

        char *val;
        PyArg_Parse(ret, "s", &val);
        string inputfile = val;

        Py_DECREF(ret);
        Py_DECREF(pargs);
        Py_DECREF(function);
        Py_DECREF(input);

        // str is a Boost Python C++ wrapper for Python strings.
//        str strStartScript(file.str().c_str());
        str strStartScript(inputfile);

        if (verbose) {
            fprintf(outfile, "Input file to run:\n%s", inputfile.c_str());
            fflush(outfile);
        }

        try {
            PyImport_AppendInittab(strdup("PsiMod"), initPsiMod);
            object objectMain(handle<>(borrowed(PyImport_AddModule("__main__"))));
            object objectDict = objectMain.attr("__dict__");
            s = strdup("import PsiMod");
            PyRun_SimpleString(s);

            object objectScriptInit = exec( strStartScript, objectDict, objectDict );
        }
        catch (error_already_set const& e)
        {
            PyErr_Print();
        }
    }
    else {
        fprintf(stderr, "Unable to run Python input file.\n");
        return;
    }

    py_psi_plugin_close_all();
}
