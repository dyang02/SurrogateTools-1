/*************************************************************************
* This program extracts weather and deposition data from MCIP and CMAQ   *
  output file. First, it extracts six daily weather variable data        *
* from MCIP files for EPIC modeling sites and creates a text run file    *
* for WXPM3020 for computing monthly climate data.                       *
* The eleven variables in EPIC daily weather file:                       *
*   1. Radiation (MJ m^02) - daily total                                 *
*   2. Tmax (C) - 2m                                                     *
*   3. Tmin (C) - 2m                                                     *
*   4. Precipitation (mm) - daily total                                  *
*   5. Relative humidity (fraction) - daily average                      *
*   6. Windspeed (m s^-1) - 10m daily average                            *
*   7. Dry oxdized N - g/ha daily total                                  *
*   8. Dry reduced N - g/ha daily total                                  *
*   9. Wet oxidized N - g/ha daily total                                 *
*   10. Wet reduced N - g/ha daily total                                 *
*   11. Wet organic N - g/ha daily total                                 *
*                                                                        *
* Second, it reads CMAQ dry and wet deposition files to extract          *
* both dry and wet N g/ha from organix and inorganic N species           *
*                                                                        *
* There are three steps in extraction:                                   *
*   1. read in EPIC site lat and long location data, project them        *
*      into MCIP data projection and convert them into column and        *
*      row in MCIP grids.                                                *
*   2. loop through each day MCIP file (METCRO2D*YYYYDDD) and            *
*      CMAQ deposition files (*.DRYDEP.YYYYMMDD and *.WETDEP*.YYYYMMDD)  *
*      to extract daily weather variables and deposition for each EPIC   *
*      site and to write them into each EPIC site daily weather file.    *
*   3. created WXPMRUN.DAT file for all generated EPIC daily             *
*      weather files in order to run WXPM3020.DRB program.               *
*      The program computes monthly weather data                         *
*   4. create new dly data set with added previous year                  * 
*      (e.g. 2002 run will contain 2001 and 2002 with data copied        *
*            from 2002)                                                  *
*                                                                        *
* Output files:                                                          *
*   1. "site_name".dly - EPIC daily weather input file                   *
*   2. EPICW2YR.2YR - for EPIC simulation and WXPM3020.DRB               *
*                     computing EPIC monthly weather                     *
*   3. Output IOAPI file                                                 *
*                                                                        *
* Written by: L. Ran                                                     *
*             2009 - 2010                                                *
*             Modified: 06/2011 => add 5 deposition variables            *
*             the Institute for the Environment at UNC, Chapel Hill      *
*             in support of the EPA CMAS Modeling.                       *
*             Modified: 05-06/2011 => modified N depsosition into        *
*             5 variables.
*                                                                        *
* Build: Adjust settings in Makefile, then 'make'                        *
* Run:   computeSiteDailyWeather.exe                                     *
*                                                                        *
* Needed environment variables:                                          *
*     GRID_PROJ -- grid domain proj4 projection definition               *
*     GRID_ROWS -- grid domain rows                                      *
*     GRID_COLUMNS -- grid domain columns                                *
*     GRID_XMIN -- grid domain xmin                                      *
*     GRID_YMIN -- grid domain ymin                                      *
*     GRID_XCELLSIZE -- grid domain x grid size                          *
*     GRID_YCELLSIZE -- grid domain y grid size                          *
*     GRID_NAME -- grid name IOAPI output                                *
*     DATA_DIR -- daily MCIP file directory                              *
*     DATA_DIR_CMAQ - daily CMAQ dry and wet depsotiion file directory   *
*     START_DATE -- start day: YYYYMMDD for EPIC modeling                *
*     END_DATE -- end day: YYYYMMDD for EPIC modeling                    *
*     EPIC_SITE_FILE -- EPIC site location file: site_name,long,lat      *
*     OUTPUT_DATA_DIR -- output directory to store created files         *
*     OUTPUT_NETCDF_FILE -- output NetCDF file for extracted values      *
*                                                                        *
*                                                                        *
* Revision history:                                                      *
*                                                                        *
* Date        Programmer                   Description of change         *
* ----        ----------                   ---------------------         *
* Aug, 2009    LR, UNC-IE                  MCIP file processing          *
* Jul-Aug. 2010LR                          Added CMAQ file processing    *
* May-June 2011 LR                         Modified N deposition output  *
**************************************************************************/
#include <ctype.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>

#include "sa_raster.h"
#include "commontools.h"
#include "geotools.h"

//gloabl variables
static void Usage();

//functions to call

void readEPICSiteFile (gridInfo grid, string inputEPICsiteFile);

void readMCIP_CMAQ_Files (gridInfo grid, string dataDir, string dataDepDir, string dayMidStr, string outputDir, const int ncid_out );

void readMCIPFile ( gridInfo grid, string mcipFile, int julianDateInt, string julianDateStr, string dayMidStr, const int ncid_out, int NDepSelect );

void readCMAQDryDFile ( gridInfo grid, string cmaqDryDFile, int julianDateInt, string julianDateStr, const int ncid_out );

void readCMAQWetDFile ( gridInfo grid, string cmaqWetDFile, int julianDateInt, string julianDateStr, const int ncid_out );

void extractDailyWeatherData ( string julianDateStr, int *dayTimeStepRange, float *mcipVars[], int numVars, 
                               const char *mcipVarName, int var_ndims, size_t *varDimSize );

void  writeComputeClimateFile ( string outputDir );   

void writeDuplicateDailyFile ( string outputDir );


//site information
vector<string>  siteNames;
vector<double>  siteLong, siteLat, siteX, siteY;
vector<int>     siteRow, siteCol;

//MCIP variable information
const int     numMCIPVars = 3;                //set max number of variables needed in computation
const char    *timeVarName = "TFLAG";     //Set MCIP variable names to read
const char    *radVarName = "RGRND";
const char    *t2VarName = "TEMP2";
const char    *precip1VarName = "RN";
const char    *precip2VarName = "RC";
const char    *q2VarName = "Q2";
const char    *spVarName = "PRSFC";
const char    *wind10VarName = "WSPD10";
const char    *dryNDName = "DNDEP";
const char    *wetNDName = "WNDEP";

//CMAQ dry deposition variables
const int   numDryDVars = 15;   //15 CMAQ variables related to dry N
std::string cmaqDryDVarNames[] = {"NO2","NO","HNO3","ANO3I","ANO3J","ANO3K","PAN","PANX","NTR","N2O5","HONO","ANH4I","ANH4J","ANH4K","NH3_Dep"};
double   cmaqDryDVarFactors[] = {0.30435, 0.46667, 0.22222, 0.22581, 0.22581, 0.22581, 0.11570, 0.11570, 0.10770, 0.25926, 0.29787,1.00000, 1.00000, 1.00000, 1.05900};

//CMAQ wet deposition variables
const int   numWetDVars = 16;  //16 CMAQ variables related to wet N
std::string cmaqWetDVarNames[] = {"NO2","NO","ANO3I","ANO3J","ANO3K","HNO3","PAN","PANX","NTR","N2O5","HONO","PNA","ANH4I","ANH4J","ANH4K","NH3"};
double   cmaqWetDVarFactors[] = {0.30435, 0.46667, 1.00000, 1.00000, 1.00000, 0.98400, 0.11570, 0.11570, 0.10770, 0.25926, 0.29787, 0.177720, 1.00000, 1.00000, 1.00000, 1.05900};

const double sigmaLevels[] = {0.50, 1.0};

std::map< int,vector< float* > >  siteDailyDataV;  //daily data string vector for each site -  11 items
int                               totalDailyItems; //total daily items stored in vector float in siteDailyDataV
vector< int* >                    dayDataV;        //YYYY, MM, DD string vector - 3 items



//IOAPI NetCDF output information
int     numOutVars = 11;

const Name variableNames[ ] = { "Radiation", "Tmax", "Tmin", "Precipitation","R_humidity","Windspeed","Dry_Oxidized_ND", 
                                 "Dry_Reduced_ND", "Wet_Oxidized_ND", "Wet_Reduced_ND", "Wet_Organic_ND" };

const Name variableUnits[ ] = { "MJ m^02", "Celsius", "Celsius", "mm", "fraction","m/s", "g/ha", "g/ha", "g/ha", "g/ha", "g/ha" };

const Line variableDescriptions[ ] = { "Daily radiation total", "Daily max T", "Daily min T", "Daily total precipitation",
                                       "Daily average relative humidity", "Daily average windspeed","Daily total dry oxidized N deposition",
                                       "Daily total dry reduced N deposition", "Daily total wet oxidized N deposition", 
                                       "Daily total wet reduced N deposition","Daily total wet organic N deposition" };

int             timeSteps = 0;             //total time steps
float           *outV1, *outV2, *outV3;    //NC output float array pointers

int 		NDepSelect;                //0=Zero, 1=Default, 2=CMAQ N deposition
float           wetDepR = 7.99;            //Mass of N = (7.99x10^-4 gm/l)(10^-3 l/cm^3)(10^-1 cm/mm)(10^8 cm^2/ha)=7.99gm/mm rainfall ha
					   //assume N mixing ratio 0.8ppm for wet default deposition computation


/***************************
*********** MAIN  **********
***************************/
int main(int nArgc, char *Argv[])
{
	/******************
	* input variables *
	*******************/
        gridInfo    grid;                  //data structure to store a modeling domain information
        string      dataDir;               //directory containing daily MCIP data files
        string      dataDepDir;            //directory containing daily CMAQ deposition data files
        string      startDate, endDate;    //date range for the EPIC daily weather data extraction
        string      inputEPICsiteFile;     //csv file contains EPIC: site_name,longitude,latitude

        string      outputDir;             //output directory to store created EPIC daily weather files
        string      outputDir_2;           //output directory to store created duplicate EPIC daily weather files
        string      outNetcdfFile;         //netCDF output file which contains all extracted daily weather data


        /***************************
        * Map projection variables *
        ***************************/
        char    *pszProj4;
        int     proj;      //WRF projection type
        string  temp_proj;

        //print program version
        printf ("\nUsing: %s\n", prog_version);


	/*********************************
	* Obtain environmental variables *
	*********************************/
        //printf ("Obtaining environment variables...\n");

        //no arguments to call this program
        if ( nArgc  > 1 )
        {
           printf( "\nError:  No arguments are nedded.\n");
           printf( "\tUsage: computeSiteDailyWeather.exe\n");
           exit( 1 );
        }

        //rows and columns of the domain
        grid.rows = atoi( getEnviVariable("GRID_ROWS") );
        grid.cols = atoi( getEnviVariable("GRID_COLUMNS") );
        printf( "\tRows=%d    Cols=%d    Total gird cells=%d\n",grid.rows,grid.cols,grid.rows*grid.cols);
        if (grid.cols <1 || grid.rows <1)
        {
            printf("\tError: grid rows and columns have to be great than 1.\n");
            exit( 1 );
        }

        //get xmin and ymin for the domain
        grid.xmin = (float) atof( getEnviVariable("GRID_XMIN") );
        grid.ymin = (float) atof( getEnviVariable("GRID_YMIN") );
        printf( "\txmin=%f    ymin=%f \n", grid.xmin, grid.ymin);

        //get x and y cell size
        grid.xCellSize = (float) atof( getEnviVariable("GRID_XCELLSIZE") );
        grid.yCellSize = (float) atof( getEnviVariable("GRID_YCELLSIZE") );
        printf( "\txcell=%.0f    ycell=%.0f \n",grid.xCellSize,grid.yCellSize);
        if (grid.xCellSize <= 0.0 || grid.yCellSize <= 0.0)
        {
            printf("\tError: domain grid cell size has to be > 0.\n");
            exit( 1 );
        }

        //max x and y
        grid.xmax = grid.xmin + grid.xCellSize * grid.cols;
        grid.ymax = grid.ymin + grid.yCellSize * grid.rows;

        //get projection info
        pszProj4 = getEnviVariable("GRID_PROJ");
        grid.strProj4 = getEnviVariable("GRID_PROJ");
        printf( "\tproj4=%s\n",grid.strProj4);

         //get WRF projection type from Proj4 string
        temp_proj = string (pszProj4);
        proj = getProjType(temp_proj);    //WRF has 4 projection types
        printf( "\tproj type = %d \n",proj);

        /***************************
        *       get Grid Name      *
        ***************************/
        grid.name = string( getEnviVariable("GRID_NAME") );
        printf("\tGrid name: %s\n",grid.name.c_str() );

        /*************************************
        *       get input data directories   *
        **************************************/
        //printf( "Getting directory containing daily MCIP files.\n");
        dataDir = string( getEnviVariable("DATA_DIR") );
        dataDir = processDirName( dataDir );  //make sure that dir ends with path separator
        printf("\tMCIP file directory: %s\n", dataDir.c_str() );
        FileExists(dataDir.c_str(), 0 );  //the dir has to exist.

        //printf( "Getting deposition selection(CMAQ depsoition default 0.8ppm, or zero).\n");
        dataDepDir = string( getEnviVariable("DATA_DIR_CMAQ") );
        printf("\tDeposition is: %s\n",dataDepDir.c_str() );

        
        //set number of variables to compute from input files
        int mcipVarNum = 7;   //including average T
        int NdepNum = 0;

        if ( dataDepDir.compare ("Zero" ) == 0 )
        {
           NDepSelect = 0;
        }
        else if ( dataDepDir.compare ("Default" ) == 0 )
        {
           NDepSelect = 1;
        }
        else
        {
           dataDepDir = processDirName( dataDepDir );  //make sure that dir ends with path separator

           FileExists(dataDepDir.c_str(), 0 );  //the dir has to exist.

           NDepSelect = 2;
           NdepNum = 5;   //get 6 N Dep variables
        }

        totalDailyItems = mcipVarNum + NdepNum; //totoal variables to compute from input files

        printf ("\tNDepSelect = %d\n", NDepSelect);


        /***********************************
        *       get day and time range     *
        ***********************************/
        //printf( "Getting day range to extract MCIP data.\n");
        //get day in YYYYMMDD
        startDate = string( getEnviVariable("START_DATE") );  
        startDate = trim( startDate );
        endDate = string( getEnviVariable("END_DATE") );
        endDate = trim( endDate );
        printf ( "\tStart date is:  %s\n",startDate.c_str() );
        printf ( "\tEnd date is:  %s\n",endDate.c_str() );
        if ( startDate.length() != 8 || endDate.length() != 8 )
        {
           printf ( "\tError: Start and End date format should be: YYYYMMDD\n" );
           exit ( 1 );
        }
        if ( atol(startDate.c_str()) == 0 ||  atol(endDate.c_str()) == 0 )
        {
           printf ( "\tError: Start and End date should be all numbers in YYYYMMDD\n" );
           exit ( 1 );
        }
        checkYYYYMMDD ( startDate );
        checkYYYYMMDD ( endDate );
       
        //printf( "Getting EPIC modeling site file.\n");
        inputEPICsiteFile = string ( getEnviVariable("EPIC_SITE_FILE") );
        inputEPICsiteFile = trim( inputEPICsiteFile );
        printf( "\tEPIC site file is:  %s\n",inputEPICsiteFile.c_str() );
        FileExists(inputEPICsiteFile.c_str(), 0 );  //the file has to exist.


        /***********************************
        *       set output dir and file    *
        ***********************************/
        //printf( "Getting output data directory.\n");
        outputDir = string( getEnviVariable("OUTPUT_DATA_DIR") );
        outputDir = trim ( outputDir );   
        outputDir = processDirName( outputDir );  //make sure that dir ends with path separator

        //set duplicate daily output dir to be: dailyWETH
        outputDir_2 = outputDir + string ( "dailyWETH/" );
        
        printf( "\tOutput data directory is: %s\n",outputDir.c_str() );
        FileExists(outputDir.c_str(), 2 );  //check the directory and create it if it is new

        FileExists(outputDir_2.c_str(), 2 );  //check the directory and create it if it is new
        


        //get netCDF output file name.  If it is NONE, NetCDF file will not be created.
        outNetcdfFile = string ( getEnviVariable( "OUTPUT_NETCDF_FILE" ) );
        outNetcdfFile = trim ( outNetcdfFile );
        FileExists(outNetcdfFile.c_str(), 3 );  //the file has to be new
              

	/***********************************************
        * Read in EPIC sites, project them into MCIP,  *
	* and compute column/row in MCIP grids         *
	***********************************************/
        readEPICSiteFile (grid, inputEPICsiteFile);
        
        //clean output directory files
        for (int i=0; i<siteRow.size(); i++)
        {
           string   siteName = siteNames[i];
           string   outputFile  = outputDir_2 + siteName + string (".dly");  //EPIC daily weather file
           FileExists(outputFile.c_str(), 3 );  //the file has to be new

           outputFile  = outputDir_2 + siteName + string (".tav");  //average T file
           FileExists(outputFile.c_str(), 3 );  //the file has to be new
        }

       
        /*****************************************************************
        *       Write IOAPI netCDF output file dimension and variableis    *
        ******************************************************************/
          
        string startDate_julian = getJulianDayStr ( startDate );
        int timeStepsD = getTimeSteps ( startDate, endDate );
          
        printf ("\tStart Time: %s   End Time: %s   DaySteps: %d\n", startDate.c_str(), endDate.c_str(), timeStepsD);

        
        /*************************************
        * Set variables to write netCDF file *
        *************************************/
        //temp_proj = string (grid.strProj4);
        proj = getProjType(temp_proj);   //WRF projection type

        float    stdLon,stdLat;
        float    trueLat1, trueLat2;
        if (proj == LCC)
        {
           trueLat1 = getValueInProj4Str ( temp_proj, "lat_1=");
           trueLat2 = getValueInProj4Str ( temp_proj, "lat_2=");
           stdLat = getValueInProj4Str ( temp_proj, "lat_0=");
           stdLon = getValueInProj4Str ( temp_proj, "lon_0=");
        }
        else if ( proj == LATLONG)
        {
           trueLat1 = 0.0;
           trueLat2 = 0.0;
           stdLat = 0.0;
           stdLon = 0.0;
        }
        else if ( proj == MERC )
        { 
           //may have issues in IOAPI definition!!

           trueLat1 = 0.0;
           trueLat2 = getValueInProj4Str ( temp_proj, "lat_ts=");
           stdLat = 0.0;
           stdLon = getValueInProj4Str ( temp_proj, "lon_0=");
        }
        else if ( proj == UPS )
        {
           //may have issues in IOAPI definition!!

           stdLat = getValueInProj4Str ( temp_proj, "lat_ts=");
           stdLon = getValueInProj4Str ( temp_proj, "lon_0=");
           if ( stdLat > 0.0 )
           {
              trueLat1 = 1.0;
              trueLat2 = 90.0;
           }
           else
           {
              trueLat1 = -1.0;
              trueLat2 = -90.0;
           } 
        }
        else
        {
           printf ("Error: Projection type is not LCC.  Need to add new projection in the program.\n");
           exit ( 1 );
        }

        const int LAYERS = 1;
        const int ROWS = grid.rows;
        const int COLUMNS = grid.cols;
        const double xcellSize  = grid.xCellSize;
        const double ycellSize  = grid.yCellSize;
        const double westEdge  = grid.xmin;
        const double southEdge = grid.ymin;
        const double centralLongitude = stdLon;
        const double centralLatitude = stdLat;
        const double lowerLatitude = trueLat1;
        const double upperLatitude = trueLat2;
        const double topPressure = 50.0;   //faked data as layer is 1
        
        const int VARIABLES = numOutVars;
        const int TIMESTEPS = timeStepsD;
        const int YYYYDDD = atoi ( startDate_julian.c_str() );
        const int startTime_att = 0;
        const int timeSteps_att = 240000;

        const Line description = "EPIC daily weather and N deposition data";

        /***************************************
        * Open and Define time step IOAPI file *
        ***************************************/
       
        int fileInfo;
        if (  proj == LCC )
        {
           fileInfo = createLambertConformalM3IOFile ( outNetcdfFile.c_str(),
                    VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize,ycellSize, westEdge, southEdge,
                    centralLongitude, centralLatitude, centralLongitude,
                    lowerLatitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNames, variableUnits, variableDescriptions,
                    grid.name.c_str(), description );
        }
        else if ( proj == LATLONG )
        {
           fileInfo = createLonLatM3IOFile ( outNetcdfFile.c_str(),
                    VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    topPressure, sigmaLevels,
                    variableNames, variableUnits, variableDescriptions,
                    grid.name.c_str(), description );
        }
        else if ( proj == MERC )
        {
           fileInfo = createEquitorialMercatorM3IOFile ( outNetcdfFile.c_str(),
                    VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude,
                    centralLongitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNames, variableUnits, variableDescriptions,
                    grid.name.c_str(), description );

        }
        else if ( proj == UPS )
        {
           fileInfo = createStereographicM3IOFile ( outNetcdfFile.c_str(),
                    VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
                    startTime_att, timeSteps_att,
                    xcellSize, ycellSize,
                    westEdge, southEdge,
                    centralLongitude, centralLatitude,
                    centralLongitude, upperLatitude,
                    topPressure, sigmaLevels,
                    variableNames, variableUnits, variableDescriptions,
                    grid.name.c_str(), description );

        } 

        const int ncid = fileInfo;
        int ok = ncid != -1;

        if ( ncid == -1 )
        {
            printf ("\tError: creating netCDf file: %s\n", outNetcdfFile.c_str() );
            exit ( 1 );
        }

        /*************************************************
        * Loop through date range to read each day MCIP  *
        * and CMAQ data for each EPIC site and to write  *
        * extracted site daily weather into              *
        *"site_name".dly file  and output NetCDF file    *
        *************************************************/
        string      dayMidStr;      //YYYYMMDD string 
        long        dayStart, dayEnd, dayMid;        //day string number
       
        dayStart = atol ( startDate.c_str() );
        dayEnd = atol ( endDate.c_str() );

        //initialize
        dayMidStr = string ( startDate );
        dayMid = dayStart;

        //set output variable dimension size in grid
        grid.dims.push_back (grid.rows);
        grid.dims.push_back (grid.cols);

        while ( dayMid <= dayEnd )
        {

           //read one day MCIP/CMAQ files and write result in output
           readMCIP_CMAQ_Files (grid, dataDir, dataDepDir, dayMidStr, outputDir_2, ncid);   

           timeSteps += 1;

           dayMidStr = getNextDayStr ( dayMidStr);
           dayMid = atol ( dayMidStr.c_str() );
        }

        /********************
        * Close netCDF file *
        ********************/
        ok = closeM3IOFile( ncid ) && ok;

        printf( "\tFinished writing epic daily time step output file: %s\n", outNetcdfFile.c_str() );

        //write duplicate daily file with added previous year
        writeDuplicateDailyFile ( outputDir_2 );   //create duplicate daily file with duplicate previous year
 
        
        //write PC bat file to compute month climate data
        writeComputeClimateFile ( outputDir );   //create WXPMRUN.DAT in output dir

        
        printf ( "\nFinishing extracting %d days MCIP and CMAQ files for EPIC modeling.\n\n", timeSteps);

}
/**************************    End of Main program   **********************/



/***********************************/
/*      read EPIC site file        */
/***********************************/
void  readEPICSiteFile (gridInfo grid, string inputEPICsiteFile)
{

     printf("\nReading EPIC site location file: %s...\n", inputEPICsiteFile.c_str() );

     /********************
     * Define projection *
     ********************/
     projPJ  proj4To, proj4From;

     //EPIC lat and long projection
     proj4From = pj_init_plus( "+proj=latlong +datum=NAD83" );  
     if ( proj4From == NULL )
     {
        printf( "\tInitializing EPIC site latlong Proj4 projection failed.\n" );
        exit( 1 );
     } 
        
     //MCIP projection
     string proj4Str = string(grid.strProj4);
     proj4To = pj_init_plus ( proj4Str.c_str() );
     if (proj4To == NULL)
     {
        printf( "\tInitializing MCIP Proj4 projection failed: %s\n", grid.strProj4 );
        exit ( 1 );
     }
 
     /********************
     * reading variables *
     ********************/
     ifstream     imageStream;             //stream to read input EPIC site file
     string       siteName;
     double       longX,latY,x,y;
     int          row,col;                 //starting from 0 
     string       lineStr,dataStr;         //for reading in site line
     projUV       xyP;
     vector<string>     vecString;
     int          i;                       //file index

     int  outsideSiteNum = 0;
     int  insideSiteNum = 0;

     i = 0;

     imageStream.open( inputEPICsiteFile.c_str() );
     if (imageStream.good() )
     {
        getline( imageStream, lineStr, '\n');
        while ( !imageStream.eof() )
        {
           lineStr = trim (lineStr);                 //get rid of spaces at edges
           i++;  //count the line number

           //get rid of empty line
           if ( lineStr.size() == 0 )
           {
              goto newloop;
           }

           //put data in string vector
           vecString = string2stringVector (lineStr, ",");
           if ( vecString.size()  < 3 )
           {
              printf( "\tError: EPIC site file should contain at least 3 items: site_name,longitude,latitude.  %d - %s\n", i,lineStr.c_str() );
              exit ( 1 );
           }

           if ( i == 1 )
           {
              printf( "\tFile header: %s\n", lineStr.c_str() );
           }
           else
           {
              //get site name
              siteName = vecString[0];

              //get longitude
              dataStr = vecString[1];
              longX = atof ( dataStr.c_str() );

              //get latitude 
              dataStr = vecString[2];
              latY = atof ( dataStr.c_str() );

              //printf ( "\tLine: %s\n", lineStr.c_str() );
              //printf ( "\tLine: %d: %s   %lf   %lf\n", i,siteName.c_str(),longX,latY );
              
 
              //project site lat and long coordinates
              xyP = projectPoint ( proj4From, proj4To, longX,latY);
              x = xyP.u;
              y = xyP.v;  
 

              //compute site row, col
              row = (int) (floor ((y - grid.ymin) / grid.yCellSize));   //start from 0
              col = (int) (floor ((x - grid.xmin) / grid.xCellSize));   //start from 0

              int gridID = row * grid.cols + col + 1;
              //printf( "\tSite: %s   longX = %lf latY = %lf  x = %lf  y=%lf  col = %d  row = %d   gridID= %d\n",siteName.c_str(), longX,latY,x,y,col,row,gridID);
          

              if ( row < 0 || row >= grid.rows || col < 0 || col >= grid.cols )
              {
                 printf ( "\t\tSite - %s is outside of MCIP domain.  No EPIC daily weather data file\n",siteName.c_str() );
                 outsideSiteNum ++;
              }
              else
              {
                 //store site info in vectors
                 siteNames.push_back ( siteName );
                 siteLong.push_back ( longX );            
                 siteLat.push_back ( latY );
                 siteX.push_back ( x );
                 siteY.push_back ( y );
                 siteCol.push_back ( col );
                 siteRow.push_back ( row );
                 insideSiteNum ++;
              }
           } //site lines

           newloop:
           getline( imageStream, lineStr);

        }  // while loop

        printf ("\tFinished reading: %d lines, %d sites inside the domain, %d sites outside the domain\n\n", i, insideSiteNum, outsideSiteNum);
        imageStream.close();

     } // if good
     else
     {
        printf ("\tError: Open file - %s\n", inputEPICsiteFile.c_str() );
        exit ( 1 );
     }

}


/***********************************/
/*      readMCIP_CMAQ_Files        */
/***********************************/
void readMCIP_CMAQ_Files (gridInfo grid, string dataDir, string dataDepDir, string dayMidStr, string outputDir, const int ncid_out)
{
     string        julianDateStr;    //YYYYDDD string
     string        julianDateStr5;   //YYDDD string
     string        dateStr6;         //YYDDMM string
     int           julianDateInt;          //mcip day number in YYYYDDD
     long          dayStart, dayEnd, dayMid;        //day string number

     string        mcipFile, cmaqDryDFile, cmaqWetDFile;
     float         *siteDailyArray;
     int           *dayArray;

     string        tmp_str;
     char          tmp_chars[25];
     int           i,j,k;    

     std::ofstream         epicOutStream;            //output text file stream

     int           *dayData;        //array to store YYYY, MM, and DD
     float         *siteDailyData;  //array to store obtained items from daily input files
     

     printf ( "\nReading MCIP and CMAQ files for day: %s...\n", dayMidStr.c_str() );


     /**************************************************/
     /*      get the day MCIP and CMAQ IOAPI files     */
     /**************************************************/

     //get date string YYMMDD
     dateStr6 = dayMidStr.substr(2, 6);

     //get Julian day YYYYDDD from YYYYMMDD
     julianDateStr = getJulianDayStr ( dayMidStr );
     julianDateInt = atoi ( julianDateStr.c_str() );
 
     //get julian date YYDDD
     julianDateStr5 = julianDateStr.substr(2, 5);

     printf ("\tDate Strings can be: %s, %s, %s, or %s\n",dayMidStr.c_str(), dateStr6.c_str(), julianDateStr.c_str(), julianDateStr5.c_str() );

     //get MCIP file: format and file METCRO2D*
     tmp_str = string ("METCRO2D"); 
     mcipFile = findOneDataFile ( dataDir, tmp_str, dayMidStr, dateStr6, julianDateStr, julianDateStr5 );

     if ( NDepSelect == 2 )
     {
        tmp_str = string ("DRYDEP");
        cmaqDryDFile = findOneDataFile ( dataDepDir, tmp_str, dayMidStr, dateStr6, julianDateStr, julianDateStr5 );

        //testing
        //cmaqDryDFile = string ( "/nas01/depts/ie/cempd/staff/lran/epic/cmaq/deposistions/CCTM_N4a_06emisv2soa_12km_wrf.DRYDEP.20020630" );

        tmp_str = string ("WETDEP");
        cmaqWetDFile = findOneDataFile ( dataDepDir, tmp_str, dayMidStr, dateStr6, julianDateStr, julianDateStr5 ); 

        //testing
        //cmaqWetDFile = string ( "/nas01/depts/ie/cempd/staff/lran/epic/cmaq/deposistions/CCTM_N4a_06emisv2soa_12km_wrf.WETDEP1.20020630" );
     }


     /**********************************/
     /*    set  EPIC daily output year */
     /**********************************/ 
     
     string epicDateStr;

     //julian date output
     /*
     string   yearStr = julianDateStr.substr(0,4);
     string   dayStr = julianDateStr.substr(4,3);

     int    yearNum = atoi ( yearStr.c_str() );
     int    dayNum =  atoi ( dayStr.c_str() );

     sprintf(tmp_chars, "%6d%6d\0", yearNum,dayNum );   //convert to EPIC format f6.2
     */

     //YYYY MM DD outout
     string   yearStr = dayMidStr.substr(0,4);
     string   monStr = dayMidStr.substr(4,2);
     string   dayStr = dayMidStr.substr(6,2);

     int   yearNum = atoi ( yearStr.c_str() );
     int   monNum = atoi ( monStr.c_str() );
     int   dayNum =  atoi ( dayStr.c_str() );

     //allocate mem to YYYY, MM, DD vector
     if ( (dayData = (int*) calloc (3, sizeof(int)) ) == NULL)
     {
         printf( "Calloc dayData failed.\n");
         exit ( 1 );
     }
     
     dayData[0] = yearNum;
     dayData[1] = monNum;
     dayData[2] = dayNum;

     dayDataV.push_back (dayData);

     
     //allocate float memeory for each site each day
     for (i=0; i<siteRow.size(); i++)
     {
        if ( (siteDailyData = (float *) calloc (totalDailyItems, sizeof(float)) ) == NULL)
        {
           printf( "Calloc siteDailyData failed.\n");
           exit ( 1 );
        }
       
        vector<float*> siteDailyData_get;
        if ( timeSteps > 0 )
        {
           siteDailyData_get = siteDailyDataV[i];   //get from each site after first day
        }
        siteDailyData_get.push_back ( siteDailyData );   //daily vector for each site

        siteDailyDataV[i] = siteDailyData_get;      //put it back to store compute values
     }


     readMCIPFile ( grid, mcipFile, julianDateInt, julianDateStr, dayMidStr, ncid_out, NDepSelect );

     
     if ( NDepSelect == 2 )
     {
        readCMAQDryDFile ( grid, cmaqDryDFile, julianDateInt, julianDateStr, ncid_out );
        readCMAQWetDFile ( grid, cmaqWetDFile, julianDateInt, julianDateStr, ncid_out );
     }
         

     printf( "\tFinished extracting daily MCIP and CMAQ file data to EPIC input files.\n" );
}



/***********************************
*      readMCIPFile                *
************************************/
void  readMCIPFile ( gridInfo grid, string mcipFile, int julianDateInt, string julianDateStr, string dayMidStr, const int ncid_out, int NDepSelect )
{
      gridInfo      ioapiGrid;   //mcip grid information
      int           i;
      string        tmp_str;
    


     /*****************************************/
     /*  read the MCIP IOAPI file info        */
     /*****************************************/
     //set NetCDF variables
     int      ncid;
     int      ndims, nvars, ngatts, unlimdimid; 
     size_t   *dimSizes, dimSize;
     char     dimName[NC_MAX_NAME+1];
     char     attName[NC_MAX_NAME+1];       //name for an attribute

     //read MCIP file
     printf("\n\tReading MCIP file: %s\n", mcipFile.c_str() );

     anyErrors( nc_open(mcipFile.c_str(), NC_NOWRITE, &ncid) );   //open MCIP file

     printf( "\tObtaining all dimension IDs in input NetCDF file...\n" );
     anyErrors( nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid) );
     printf("\tMCIP file has: %d dims, %d variables, %d global attributes, %d unlimited variable ID\n", ndims, nvars,ngatts,unlimdimid);

     //store dim size in an arrary
     dimSizes = (size_t *) malloc(sizeof(size_t) * ndims);
     if ( dimSizes == NULL )
     { 
        printf ( "\t Memory allocation malloc failed for dimSizes\n" );
        exit ( 1 );
     }

     for (i=0; i<ndims; i++)
     {
        anyErrors(  nc_inq_dim(ncid, i, dimName, &dimSize ) );
        dimSizes[i]= dimSize;
        printf("\tMCIP file dimension: dimName = %10s   dimSize = %d\n",dimName,dimSize);
     }

     /**********************************
     *   check MCIP grid information   *
     ***********************************/
     //compare with input grid information
     getGridInfofromNC ( ncid , &ioapiGrid, "IOAPI" );
   
     //printGridInfo ( grid );
     printGridInfo ( ioapiGrid );
    
     if ( ! sameGrids (grid, ioapiGrid ) )
     {
        printf ( "\tError: User defined grids and MCIP file grids are different.\n" );
        exit ( 1 );
     }


     /*****************************
     * Define variables to read   *
     ******************************/ 
     char       var_name[NC_MAX_NAME+1];          //variable name
     int        var_id;                           //variable id
     nc_type    var_type;                         // variable type 
     int        var_ndims;                        // number of dims 
     int        var_dimids[NC_MAX_VAR_DIMS];      // dimension IDs for read in array
     int        var_dimid;
     int        var_natts;                        // number of attributes 
      
     size_t     tDimSize[NC_MAX_VAR_DIMS];        // dimensions for TFLAG 
     size_t     varDimSize[NC_MAX_VAR_DIMS];      // dimensions for a variable

     int        dayTimeStepRange[2];              //Index range for the day time steps
     float      timeStepMins;                     //time step in minutes                 
     int        tStep;                            //obtained time step in HHMMSS

     ncVarData  mcipData;                             
     int        *timeVar;
     float      *mcipVars[numMCIPVars];                //number of array data needed in computation
     float      *t2Var;


     /**********************************
     *   Read time step int variable   *
     **********************************/
 
     //get var ID
     nc_inq_varid (ncid, timeVarName, &var_id);

     //get an input var infor
     anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) ); 
     printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );

     readIOAPIVar ( &mcipData, ncid, var_id, timeVarName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);
     timeVar = mcipData.intData;

     //find the time step index range for the day and the variable
     //all MCIP variables have the same time steps - call just once
     getDayTimeStepRange ( dayTimeStepRange, julianDateInt, 0, timeVar, var_ndims, varDimSize ); //no TFLAG and starts at 0
     //printf ("\tstep1 = %d  step2 = %d\n", dayTimeStepRange[0], dayTimeStepRange[1] );

     //get time step from global attribute TSTEP
     anyErrors ( nc_get_att_int ( ncid, NC_GLOBAL, "TSTEP", &tStep) );
     timeStepMins = convertMCIPTimeStep2Mins ( tStep );
     printf ("\tTime Step: %d  ( int HHMMSS)   %f (Minutes)\n", tStep, timeStepMins );

    
     /************************************
     *   Read Radiation float variable   *
     *   convert  WATTS/M**2 to MJ m^02  *
     *   daily total                     *
     ************************************/
     //allocate memory for output array
     int totalSize = grid.rows * grid.cols;
     
     outV1 = allocateFloatDataArrayMemory ( totalSize );
     fillFloatArrayMissingValue ( totalSize, outV1 );
     
     //get var ID
     nc_inq_varid (ncid, radVarName, &var_id);

     //get an input var infor
     anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) );
     printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );
     readIOAPIVar ( &mcipData, ncid, var_id, radVarName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);

     mcipVars[0] = mcipData.floatData;
     extractDailyWeatherData ( julianDateStr, dayTimeStepRange, mcipVars, 1, radVarName, var_ndims, varDimSize );
     free ( mcipVars[0] ); 

     //write IOAPI array 
     int ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[0], outV1 );
     printf( "\tWrote Radiation in output NetCDF file\n\n" );

    
     /************************************
     *   Read Temp 2m float variable     *
     *   convert K to C                  *
     *   daily Min, Max, Average         *
     ************************************/
     //allocate memory for NetCDF output array
     outV2 = allocateFloatDataArrayMemory ( totalSize );
     fillFloatArrayMissingValue ( totalSize, outV2 );
     fillFloatArrayMissingValue ( totalSize, outV1 ); 
     
     //get var ID
     nc_inq_varid (ncid, t2VarName, &var_id);

     //get an input var infor
     anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) );
     printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );
     readIOAPIVar ( &mcipData, ncid, var_id, t2VarName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);

     mcipVars[0] = mcipData.floatData;
     extractDailyWeatherData ( julianDateStr, dayTimeStepRange, mcipVars, 1, t2VarName, var_ndims, varDimSize );
     t2Var = mcipVars[0];   //save this variable to compute RH
   
     ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[1], outV1 );
     printf( "\tWrote Daily max T in output NetCDF file\n\n" );
     
     ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[2], outV2 );
     printf( "\tWrote Daily min T in output NetCDF file\n\n" );
     
     free ( outV2 );   //free Tmin array
     

     /******************************************
     *   Read precipitation float variables    *
     *   convert cm to mm                      *
     *   daily total: RN+RC                    *
     *******************************************/
     //initialize NetCDF output array
     fillFloatArrayMissingValue ( totalSize, outV1 );
      
     //get var ID for RN
     nc_inq_varid (ncid, precip1VarName, &var_id);

     //get an input var infor
     anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) );
     printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );
     readIOAPIVar ( &mcipData, ncid, var_id, precip1VarName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);
     mcipVars[0] = mcipData.floatData;


     //get var ID for RC
     nc_inq_varid (ncid, precip2VarName, &var_id);

     //get an input var infor
     anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) );
     printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );
     readIOAPIVar ( &mcipData, ncid, var_id, precip2VarName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);
     mcipVars[1] = mcipData.floatData;

     extractDailyWeatherData ( julianDateStr, dayTimeStepRange, mcipVars, 2, precip1VarName, var_ndims, varDimSize );
     free ( mcipVars[0] );
     free ( mcipVars[1] );
    
     ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[3], outV1 );
     printf( "\tWrote Daily total precipitation in output NetCDF file\n\n" );


     /*******************************************
     *   Write N deposition array for:          *
     *   NDepSelect = 0 or 1                    *
     *******************************************/
     if ( NDepSelect != 2 )
     {
        printf( "\nOutput Zero or Default deposition arrays\n");

        outV2 = allocateFloatDataArrayMemory ( totalSize );     
        for (i=6; i<=10; i++)
        {
           if ( !(i == 8  && NDepSelect == 1 ) )
           {
               // all 0 N deposition
               ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[i], outV2 );
               printf( "\tWrote Daily total %s in output NetCDF file\n\n", variableNames[i] );
           }
    
           if ( i == 8  && NDepSelect == 1 )
           {
  
              // compute woDEP based on precipitation if NDepSelect = 1
              for ( int j=0; j<totalSize; j++ ) 
              {
                 float precipVal = outV1[j];  //get precipitation
                 if ( precipVal != MISSIING_VALUE_IOAPI )
                 {
                    float woDEP = precipVal * wetDepR;  //g-N/ha/dy ratio unit
                    outV2[j] = woDEP;
                 }
              }  // j 

              ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[8], outV2 );
              printf( "\tWrote Daily total %s in output NetCDF file\n\n", variableNames[8] );
           }
        } //i
       
       free ( outV2 );
     }


    /***********************************************
     *   Read relative humidity float variables    *
     *   convert cm to mm                          *
     *   daily average fraction                    *
     **********************************************/
     //initialize NetCDF output array
     fillFloatArrayMissingValue ( totalSize, outV1 );
     
     //set T2
     mcipVars[0] = t2Var;

     //get var ID for Q2 
     nc_inq_varid (ncid, q2VarName, &var_id);

     //get an input var infor
     anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) ); 
     printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );
     readIOAPIVar ( &mcipData, ncid, var_id, q2VarName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);
     mcipVars[1] = mcipData.floatData;
     
     
     //get var ID for surface pressure 
     nc_inq_varid (ncid, spVarName, &var_id);

     //get an input var infor 
     anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) );
     printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );
     readIOAPIVar ( &mcipData, ncid, var_id, spVarName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);
     mcipVars[2] = mcipData.floatData;
        
     extractDailyWeatherData ( julianDateStr, dayTimeStepRange, mcipVars, 3, q2VarName, var_ndims, varDimSize );
     free ( mcipVars[0] );
     free ( mcipVars[1] );
     free ( mcipVars[2] ); 

     ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[4], outV1 );
     printf( "\tWrote Daily average relative humidity in output NetCDF file\n\n" );
     

     /*******************************************
     *   Read wind speed 10m float variable     *
     *   daily Average                          *
     *******************************************/
     //initialize NetCDF output array
     fillFloatArrayMissingValue ( totalSize, outV1 );
     
     //get var ID
     nc_inq_varid (ncid, wind10VarName, &var_id);

     //get an input var infor
     anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) );
     printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );
     readIOAPIVar ( &mcipData, ncid, var_id, wind10VarName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);
     mcipVars[0] = mcipData.floatData;

     extractDailyWeatherData ( julianDateStr, dayTimeStepRange, mcipVars, 1, wind10VarName, var_ndims, varDimSize );
     free ( mcipVars[0] );
    
     ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[5], outV1 );
     printf( "\tWrote Daily average windspeed in output NetCDF file\n\n" );
     
     free ( outV1 );     


     /********************
     * Close netCDF file *
     ********************/
     anyErrors( nc_close(ncid) );

}


/*****************************************
*      read CMAQ dry deposition file     *
******************************************/
void readCMAQDryDFile ( gridInfo grid, string cmaqDryDFile, int julianDateInt, string julianDateStr, const int ncid_out)
{


      gridInfo      ioapiGrid;   //mcip grid information
      int           i;
      string        tmp_str;
    


     /*****************************************/
     /*  read the CMAQ IOAPI file info        */
     /*****************************************/
     //set NetCDF variables
     int      ncid;
     int      ndims, nvars, ngatts, unlimdimid; 
     size_t   *dimSizes, dimSize;
     char     dimName[NC_MAX_NAME+1];
     char     attName[NC_MAX_NAME+1];       //name for an attribute

     //read CMAQ file
     printf("\n\tReading CMAQ dry deposition file: %s\n", cmaqDryDFile.c_str() );

     anyErrors( nc_open(cmaqDryDFile.c_str(), NC_NOWRITE, &ncid) );   //open CMAQ dry file

     printf( "\tObtaining all dimension IDs in input NetCDF file...\n" );
     anyErrors( nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid) );
     printf("\tCMAQ dry dep. file has: %d dims, %d variables, %d global attributes, %d unlimited variable ID\n", ndims, nvars,ngatts,unlimdimid);

     //store dim size in an arrary
     dimSizes = (size_t *) malloc(sizeof(size_t) * ndims);
     if ( dimSizes == NULL )
     { 
        printf ( "\t Memory allocation malloc failed for dimSizes\n" );
        exit ( 1 );
     }

     for (i=0; i<ndims; i++)
     {
        anyErrors(  nc_inq_dim(ncid, i, dimName, &dimSize ) );
        dimSizes[i]= dimSize;
        printf("\tCMAQ dry dep. file dimension: dimName = %10s   dimSize = %d\n",dimName,dimSize);
     }

     /*******************************************
     *   check CMAQ dry dep. grid information   *
     ********************************************/
     //compare with input grid information
     getGridInfofromNC ( ncid , &ioapiGrid, "IOAPI" );
   
     //printGridInfo ( grid );
     printGridInfo ( ioapiGrid );
    
     if ( ! sameGrids (grid, ioapiGrid ) )
     {
        printf ( "\tError: User defined grids and CMAQ dry dep. file grids are different.\n" );
        exit ( 1 );
     }


     /*****************************
     * Define variables to read   *
     ******************************/ 
     char       var_name[NC_MAX_NAME+1];          //variable name
     int        var_id;                           //variable id
     nc_type    var_type;                         // variable type 
     int        var_ndims;                        // number of dims 
     int        var_dimids[NC_MAX_VAR_DIMS];      // dimension IDs for read in array
     int        var_dimid;
     int        var_natts;                        // number of attributes 
      
     size_t     tDimSize[NC_MAX_VAR_DIMS];        // dimensions for TFLAG 
     size_t     varDimSize[NC_MAX_VAR_DIMS];      // dimensions for a variable

     int        dayTimeStepRange[2];              //Index range for the day time steps
     float      timeStepMins;                     //time step in minutes                 
     int        tStep;                            //obtained time step in HHMMSS

     ncVarData  cmaqData;                             
     int        *timeVar;
     float      *cmaqVars[numDryDVars];                //number of array data needed in computation
     float      *t2Var;


     /**********************************
     *   Read time step int variable   *
     **********************************/
 
     //get var ID
     nc_inq_varid (ncid, timeVarName, &var_id);

     //get an input var infor
     anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) ); 
     printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );

     readIOAPIVar ( &cmaqData, ncid, var_id, timeVarName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);
     timeVar = cmaqData.intData;

     /*
     //find the time step index range for the day and the variable
     //all CMAQ variables have the same time steps - call just once
     getDayTimeStepRange ( dayTimeStepRange, julianDateInt, 0, timeVar, var_ndims, varDimSize ); //no TFLAG and starts at 0
     //printf ("\tstep1 = %d  step2 = %d\n", dayTimeStepRange[0], dayTimeStepRange[1] );
     */

     dayTimeStepRange[0] = 0;
     dayTimeStepRange[1] = 23;

     //get time step from global attribute TSTEP
     anyErrors ( nc_get_att_int ( ncid, NC_GLOBAL, "TSTEP", &tStep) );
     timeStepMins = convertMCIPTimeStep2Mins ( tStep );
     printf ("\tTime Step: %d  ( int HHMMSS)   %f (Minutes)\n", tStep, timeStepMins );
     

     /***************************************
     *   Read numDryDVars variables need to compute  *
     *   total dry N deposition             *
     *   daily total g/ha                  *
     ****************************************/
     //allocate memory for NetCDF output array
     int totalSize = grid.rows * grid.cols;
        
     //dry oxidized N Dep 
     outV1 = allocateFloatDataArrayMemory ( totalSize );
     fillFloatArrayMissingValue ( totalSize, outV1 );
        
     //dry reduced N Dep
     outV2 = allocateFloatDataArrayMemory ( totalSize );
     fillFloatArrayMissingValue ( totalSize, outV2 );
        
     for ( i=0; i<numDryDVars; i++)
     {

        printf ("\n\t%d: %s    %.5lf\n", i, cmaqDryDVarNames[i].c_str(), cmaqDryDVarFactors[i] );

        //get var ID
        int ncStatus;
        ncStatus = nc_inq_varid (ncid, cmaqDryDVarNames[i].c_str(), &var_id);     

         //handle last variable: NH3_Dep or NH3
        if ( ncStatus != NC_NOERR && i != numDryDVars-1 )
        {
           printf ("\tError: getting netCDF variable ID for variable: %s.\n", cmaqDryDVarNames[i].c_str() );
           exit ( 1 );
        }
  
        if ( ncStatus != NC_NOERR && i == numDryDVars-1 )
        {
           //try NH3
           string  diffNH3Str = string ( "NH3" );
           cmaqDryDVarNames[i] = diffNH3Str;
             
           ncStatus = nc_inq_varid (ncid, cmaqDryDVarNames[i].c_str(), &var_id);
           if ( ncStatus != NC_NOERR )
           {
              printf ("\tError: getting netCDF variable ID for variable: %s.\n", cmaqDryDVarNames[i].c_str() );
              exit ( 1 );
           }
        }


        //get an input var infor
        anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) );
        printf( "\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );

        readIOAPIVar ( &cmaqData, ncid, var_id, cmaqDryDVarNames[i].c_str(), var_type, var_ndims, var_dimids, dimSizes, varDimSize);
        cmaqVars[i] = cmaqData.floatData;
     }
        
     extractDailyWeatherData ( julianDateStr, dayTimeStepRange, cmaqVars, numDryDVars, dryNDName, var_ndims, varDimSize );

     for ( i=0; i<numDryDVars; i++)
     {
        free ( cmaqVars[i] );
     }
        
     //write NetCDF output array
     //define array to write for one time step
     int ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[6], outV1 );
     printf( "\tWrote Daily total dry oxidized N deposition in output NetCDF file\n\n" );
        
     ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[7], outV2 );
     printf( "\tWrote Daily total dry reduced N deposition in output NetCDF file\n\n" );

     free ( outV1 );
     free ( outV2 );
     
     /********************
     * Close netCDF file *
     ********************/
     anyErrors( nc_close(ncid) );

}


/*****************************************
*      read CMAQ wet deposition file     *
******************************************/
void readCMAQWetDFile ( gridInfo grid, string cmaqWetDFile, int julianDateInt, string julianDateStr, const int ncid_out )
{


      gridInfo      ioapiGrid;   //mcip grid information
      int           i;
      string        tmp_str;
    


     /*****************************************/
     /*  read the CMAQ IOAPI file info        */
     /*****************************************/
     //set NetCDF variables
     int      ncid;
     int      ndims, nvars, ngatts, unlimdimid; 
     size_t   *dimSizes, dimSize;
     char     dimName[NC_MAX_NAME+1];
     char     attName[NC_MAX_NAME+1];       //name for an attribute

     //read CMAQ file
     printf("\n\tReading CMAQ wet deposition file: %s\n", cmaqWetDFile.c_str() );

     anyErrors( nc_open(cmaqWetDFile.c_str(), NC_NOWRITE, &ncid) );   //open CMAQ wet file

     printf( "\tObtaining all dimension IDs in input NetCDF file...\n" );
     anyErrors( nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid) );
     printf("\tCMAQ wet dep. file has: %d dims, %d variables, %d global attributes, %d unlimited variable ID\n", ndims, nvars,ngatts,unlimdimid);

     //store dim size in an arrary
     dimSizes = (size_t *) malloc(sizeof(size_t) * ndims);
     if ( dimSizes == NULL )
     { 
        printf ( "\t Memory allocation malloc failed for dimSizes\n" );
        exit ( 1 );
     }

     for (i=0; i<ndims; i++)
     {
        anyErrors(  nc_inq_dim(ncid, i, dimName, &dimSize ) );
        dimSizes[i]= dimSize;
        printf("\tCMAQ wet dep. file dimension: dimName = %10s   dimSize = %d\n",dimName,dimSize);
     }

     /*******************************************
     *   check CMAQ wet dep. grid information   *
     ********************************************/
     //compare with input grid information
     getGridInfofromNC ( ncid , &ioapiGrid, "IOAPI" );
   
     //printGridInfo ( grid );
     printGridInfo ( ioapiGrid );
    
     if ( ! sameGrids (grid, ioapiGrid ) )
     {
        printf ( "\tError: User defined grids and CMAQ wet dep. file grids are different.\n" );
        exit ( 1 );
     }


     /*****************************
     * Define variables to read   *
     ******************************/ 
     char       var_name[NC_MAX_NAME+1];          //variable name
     int        var_id;                           //variable id
     nc_type    var_type;                         // variable type 
     int        var_ndims;                        // number of dims 
     int        var_dimids[NC_MAX_VAR_DIMS];      // dimension IDs for read in array
     int        var_dimid;
     int        var_natts;                        // number of attributes 
      
     size_t     tDimSize[NC_MAX_VAR_DIMS];        // dimensions for TFLAG 
     size_t     varDimSize[NC_MAX_VAR_DIMS];      // dimensions for a variable

     int        dayTimeStepRange[2];              //Index range for the day time steps
     float      timeStepMins;                     //time step in minutes                 
     int        tStep;                            //obtained time step in HHMMSS

     ncVarData  cmaqData;                             
     int        *timeVar;
     float      *cmaqVars[numWetDVars];                //number of array data needed in computation
     float      *t2Var;


     /**********************************
     *   Read time step int variable   *
     **********************************/
 
     //get var ID
     nc_inq_varid (ncid, timeVarName, &var_id);

     //get an input var infor
     anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) ); 
     printf( "\n\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );

     readIOAPIVar ( &cmaqData, ncid, var_id, timeVarName, var_type, var_ndims, var_dimids, dimSizes, varDimSize);
     timeVar = cmaqData.intData;

     /*
     //find the time step index range for the day and the variable
     //all CMAQ variables have the same time steps - call just once
     getDayTimeStepRange ( dayTimeStepRange, julianDateInt, 0, timeVar, var_ndims, varDimSize ); //no TFLAG and starts at 0
     //printf ("\tstep1 = %d  step2 = %d\n", dayTimeStepRange[0], dayTimeStepRange[1] );
     */

     dayTimeStepRange[0] = 0;
     dayTimeStepRange[1] = 23;

     //get time step from global attribute TSTEP
     anyErrors ( nc_get_att_int ( ncid, NC_GLOBAL, "TSTEP", &tStep) );
     timeStepMins = convertMCIPTimeStep2Mins ( tStep );
     printf ("\tTime Step: %d  ( int HHMMSS)   %f (Minutes)\n", tStep, timeStepMins );
     

     /************************************************
     *   Read numWetDVars variables need to compute  *
     *   total wet N deposition                      *
     *   daily total g/ha                            *
     ************************************************/
     //allocate memory for NetCDF output array
     int totalSize = grid.rows * grid.cols;
        
     //wet oxidized N Dep
     outV1 = allocateFloatDataArrayMemory ( totalSize );
     fillFloatArrayMissingValue ( totalSize, outV1 );
        
     //wet reduced N Dep
     outV2 = allocateFloatDataArrayMemory ( totalSize );
     fillFloatArrayMissingValue ( totalSize, outV2 );
        
     //wet organic N Dep
     outV3 = allocateFloatDataArrayMemory ( totalSize );
     fillFloatArrayMissingValue ( totalSize, outV3 );
        
     for ( i=0; i<numWetDVars; i++)
     {

        printf ("\n\t%d: %s    %.5lf\n", i, cmaqWetDVarNames[i].c_str(), cmaqWetDVarFactors[i] );
        //get var ID
        nc_inq_varid (ncid, cmaqWetDVarNames[i].c_str(), &var_id);

        //get an input var infor
        anyErrors( nc_inq_var( ncid, var_id, var_name, &var_type, &var_ndims, var_dimids, &var_natts) );
        printf( "\tVariable name = %s    var_type = %d    ndims=%d\n",var_name, var_type, var_ndims );

        readIOAPIVar ( &cmaqData, ncid, var_id, cmaqWetDVarNames[i].c_str(), var_type, var_ndims, var_dimids, dimSizes, varDimSize);
        cmaqVars[i] = cmaqData.floatData;
     }
        
     extractDailyWeatherData ( julianDateStr, dayTimeStepRange, cmaqVars, numWetDVars, wetNDName, var_ndims, varDimSize );

     for ( i=0; i<numWetDVars; i++)
     {
        free ( cmaqVars[i] );
     }
        
      //write NetCDF output array
     int ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[8], outV1 );
     printf( "\tWrote daily total wet oxidized N deposition in output NetCDF file\n\n" );

     ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[9], outV2 );
     printf( "\tWrote daily total wet reduced N deposition in output NetCDF file\n\n" );
        
     ok = writeM3IODataForTimestep ( ncid_out, timeSteps, variableNames[10], outV3 );
     printf( "\tWrote daily total wet organic N deposition in output NetCDF file\n\n" );
        
     free ( outV1 );
     free ( outV2 );
     free ( outV3 );
     

     /********************
     * Close netCDF file *
     ********************/
     anyErrors( nc_close(ncid) );

}



/***********************************
*     extractDailyWeatherData      *
************************************/
void extractDailyWeatherData ( string julianDateStr, int *dayTimeStepRange, float *mcipVars[], int numVars, const char *mcipVarName, int var_ndims, size_t *varDimSize )
{
   int     i, j, k, index, gridIndex;
   double  epicValue;
   float   tMin, tMax, tAve;
   double  oNDep, rNDep, gNDep;  //N depsotions - oxidized, reduced, and organic
   int     varItemPos;           //item position variable positionposition of ariables in 

   double  SVP1 = 0.6112;
   double  SVP2 = 17.67;
   double  SVP3 = 29.65;
   double  SVPT0 = 273.15;
   double  EP_2 = 0.622;

   char    tmp_chars[20];

   double  minDep = 999999.0;
   double  maxDep = -999999.0;

   
   printf ( "\tjulianDateStr=%s  step1=%d  step2=%d  numVars=%d  varName=%s  var_ndimes=%d\n",julianDateStr.c_str(), dayTimeStepRange[0],
   	     dayTimeStepRange[1],numVars,mcipVarName,var_ndims);
   //printf ( "\tD1=%d   D2=%d   D3=%d    D4=%d\n",varDimSize[0],varDimSize[1],varDimSize[2],varDimSize[3]);
   
   //checking data
 /*  for (j = 900; j<1000; j++)
   {
      printf ("\t\t");
      for ( i=0; i<numVars; i++)
       printf ("VAR%d=%f\n", mcipVars[i][j]);
     printf ("\n");
   }
 */  
  
   if ( var_ndims != 4 )
   {
      printf ( "\tError: Only processing IOAPI 4D float variable.\n");
      exit ( 1 );
   }


   //loop through sites and compute each site variable data
   for ( i=0; i<siteRow.size(); i++ )
   {
      epicValue = 0.0;
      tMin = 10000.0; 
      tMax = -10000.0;
      tAve = 0.0;
      oNDep = 0.0;
      rNDep = 0.0;
      gNDep = 0.0;

      //loop through day time steps
      for ( j=dayTimeStepRange[0]; j<=dayTimeStepRange[1]; j++ )
      {
         index = j * varDimSize[1] * varDimSize[2] * varDimSize[3] + siteRow[i] * varDimSize[3] + siteCol[i];   

         if ( strcmp (mcipVarName, radVarName) == 0 )  
         {
            varItemPos = 0;  

            //printf ("\tCompute daily radiation...\n" );
            epicValue += mcipVars[0][index] * 3600.00;  //convert second unit to hour unit 
            //printf ("\t%d  Value =%15.12f    epicValue=%15.10lf\n",j,mcipVars[0][index],epicValue);

         }
         else if ( strcmp (mcipVarName, t2VarName) == 0 )
         {
            varItemPos = 1;

            //printf ("\tCompute daily T at 2m...\n" );
            tMin = min (tMin, mcipVars[0][index] );
            tMax = max ( tMax, mcipVars[0][index] );
            tAve += mcipVars[0][index];
            //printf ("\t%d  Value =%15.12f  tMin=%f  tMax=%f   tAve=%f\n",j,mcipVars[0][index],tMin,tMax,tAve);
         }
         else if ( strcmp (mcipVarName, precip1VarName) == 0 )
         {
            varItemPos = 4;

            //printf ("\tCompute daily precipitation...\n" );
            epicValue += ( mcipVars[0][index] + mcipVars[1][index] ) * 10.0;
            //printf ("\t%d  RN=%15.12f  RC=%15.12f   epicValue=%15.10lf\n",j,mcipVars[0][index],mcipVars[1][index],epicValue);
         }
         else if ( strcmp (mcipVarName, q2VarName) == 0 )
         {
            varItemPos = 5;

            //printf ("\tCompute daily average relative humidity fraction...\n" );
            double   VAPPRS = SVP1 * exp( SVP2 * (mcipVars[0][index] - SVPT0) / (mcipVars[0][index] - SVP3) );
            double   QSBT  = EP_2 * VAPPRS / (mcipVars[2][index]/1000.0 - VAPPRS);
            double   RH2   = mcipVars[1][index] / QSBT;
            epicValue += RH2;
           // printf ( "\ttstep=%d   RH2=%lf\n", j, epicValue);
         }
         else if ( strcmp (mcipVarName, wind10VarName) == 0 )
         {
            varItemPos = 6;

            //printf ("\tCompute daily average windspeed...\n" );
            epicValue += mcipVars[0][index]; 
            //printf ("\t%d  Value =%15.12f    epicValue=%15.10lf\n",j,mcipVars[0][index],epicValue);
         }
         else if ( strcmp (mcipVarName, dryNDName) == 0 )
         {
         /*	 std::string cmaqDryDVarNames[] = {"NO2","NO","HNO3","ANO3I","ANO3J","ANO3K","PAN","PANX","NTR","N2O5","HONO","ANH4I","ANH4J","ANH4K","NH3"};
double   cmaqDryDVarFactors[] = {0.30435, 0.46667, 0.22222, 0.22581, 0.22581, 0.22581, 0.11570, 0.11570, 0.10770, 0.25926, 0.29787,1.00000, 1.00000, 1.00000, 1.05900};
           c.  dry oxidized N:  DD_OXN_TOT=DD_OXN_NOX_DD_OXN_TNO3_DD_OXN_ORGN_DD_OXN_OTHR
           d.  dry reduced N:  DD_REDN_TOT=0.77778*DDEP_NHX where DDEP_NHX=ANH4I+ANH4J+ANH4K+1.059*NH3
*/

            varItemPos = 7;
         	 
            double ddepNHX = 0.0;
            for ( k=0; k<=10; k++)
            {
               oNDep += mcipVars[k][index] * cmaqDryDVarFactors[k];
            } 
            for (k=11; k<=14; k++)
            {
               ddepNHX += mcipVars[k][index] * cmaqDryDVarFactors[k];   
            }
            rNDep += ddepNHX * 0.77778;                 
         }
         else if ( strcmp (mcipVarName, wetNDName) == 0 )
         {
         	 /*
         	 const int   numWetDVars = 16;  //16 CMAQ variables related to wet N
std::string cmaqWetDVarNames[] = {"NO2","NO","ANO3I","ANO3J","ANO3K","HNO3","PAN","PANX","NTR","N2O5","HONO","PNA","ANH4I","ANH4J","ANH4K","NH3"};
double   cmaqWetDVarFactors[] = {0.30435, 0.46667, 1.00000, 1.00000, 1.00000, 0.98400, 0.11570, 0.11570, 0.10770, 0.25926, 0.29787, 0.177720, 1.00000, 1.00000, 1.00000, 1.05900};

           a.  wet oxidized N: WD_OXN_TOT = WD_OXN_NOX_WD_OXN_TNO3_WD_OXN_ORGN_WD_OXN_OTHR
           b.  wet reduced N: WD_REDN_TOT=0.77778*WDEP_NHX where WDEP_NHX=ANH4I+ANH4J+ANH4K+1.059*NH3
           e.  wet organic N:  .33*(WD_OXN_TOT+WD_REDN_TOT)
*/
         	 
            varItemPos = 9;
         	 
            double wdepTNO3 = 0.0;
            double wdepNHX = 0.0;
            for ( k=0; k<=1; k++)
            {
               oNDep += mcipVars[k][index] * cmaqWetDVarFactors[k];
            }
            for (k=2; k<=5; k++)
            {
               wdepTNO3 += mcipVars[k][index] * cmaqWetDVarFactors[k];  
            }
            oNDep += wdepTNO3 * 0.22581;

            for ( k=6; k<=11; k++)
            {
               oNDep += mcipVars[k][index] * cmaqWetDVarFactors[k];
            }
            
            for ( k=12; k<=15; k++)
            {
               wdepNHX += mcipVars[k][index] * cmaqWetDVarFactors[k];
            }
            rNDep += wdepNHX * 0.77778;         
         }
         else
         {
            printf ( "\tNo matched variable name in extractDailyWeatherData function: %s\n", mcipVarName );
            exit ( 1 );
         }
         
      } // j - time steps
      
      //process daily data to right units
      if ( strcmp (mcipVarName, radVarName) == 0 )
      {
         epicValue = epicValue / 1000000.00;    //convert J to MJ
      }
      else if ( strcmp (mcipVarName, t2VarName) == 0 )
      {
          //convert K to C
          tMin = tMin - 273.15;
          tMax = tMax - 273.15;
          tAve = tAve / (dayTimeStepRange[1] - dayTimeStepRange[0] + 1) - 273.15;
      }
      else if ( strcmp (mcipVarName, q2VarName) == 0 || strcmp (mcipVarName, wind10VarName) == 0 )
      {
         epicValue =  epicValue  / (dayTimeStepRange[1] - dayTimeStepRange[0] + 1);
         if ( strcmp (mcipVarName, q2VarName) == 0 && epicValue > 1.0 )
         {
            epicValue = 1.0;    //has to be <= 1.0
         }
      }
      else if ( strcmp (mcipVarName, dryNDName) == 0 || strcmp (mcipVarName, wetNDName) == 0 )
      {
         oNDep = oNDep * 1000.0;   //convert kg/ha to g/ha
      	 rNDep = rNDep * 1000.0;   //convert kg/ha to g/ha
      	 
         epicValue = (oNDep + rNDep);
      	 minDep = min (minDep, epicValue);
         maxDep = max (maxDep, epicValue);
         if ( strcmp (mcipVarName, wetNDName) == 0 )
         {
            gNDep = 0.33 *  epicValue;
         }
      }


      //get daily float pointer vector for i site 
      vector<float*> siteDailyData_get = siteDailyDataV[i]; 

      //get current time step float array pointer
      float * dailyV = siteDailyData_get[timeSteps];


      //get gridIndex for NetCDF output array
      int gridIndex = siteRow[i] * varDimSize[3] + siteCol[i];


      if ( strcmp (mcipVarName, t2VarName) == 0 )
      {
         dailyV[ varItemPos ] =  tMax;
         dailyV[ varItemPos + 1 ] = tMin;
         dailyV[ varItemPos + 2 ] = tAve;

         outV1[gridIndex] = tMax;
         outV2[gridIndex] = tMin;
      }
      else if ( strcmp (mcipVarName, dryNDName) == 0 )
      {
         dailyV[ varItemPos ] = oNDep;
         dailyV[ varItemPos + 1 ] = rNDep;
         
         outV1[gridIndex] = oNDep;
         outV2[gridIndex] = rNDep;
         	      
      }
      else if ( strcmp (mcipVarName, wetNDName) == 0 )
      {
         dailyV[ varItemPos ] = oNDep;
         dailyV[ varItemPos + 1 ] = rNDep;
         dailyV[ varItemPos + 2 ] = gNDep;

         outV1[gridIndex] = oNDep;
         outV2[gridIndex] = rNDep;
         outV3[gridIndex] = gNDep;
      }
      else
      {
         dailyV[ varItemPos ] = epicValue;

         outV1[gridIndex] = epicValue;
      }	      

      //assign data back to hash table
      siteDailyData_get.at(timeSteps) = dailyV;
      siteDailyDataV[i] = siteDailyData_get;

   }  //i - sites

   printf ("\tminDep=%lf   maxDep=%lf\n", minDep, maxDep);
}


/********************************************
* Write EPICW2YR.2YR to run:                *
*       1. EPIC application model           *
*       2. WXPM3020 for generating          *
*          "site".INP to be named in        *
*          monthly weather list file        *
*          like WPM1US.DAT                  *
********************************************/
void  writeComputeClimateFile ( string outputDir )
{
     
   string             outputFile;
   std::ofstream      outputStream;           //output text file stream
   char               temp_char[100];

   printf ( "\nWriting EPICW2YR.2YR file for all generated EPIC daily weather files...\n" );

   outputFile = outputDir + string ( "EPICW2YR.2YR" );
   FileExists( outputFile.c_str(), 3 );  //the file has to be new

   outputStream.open( outputFile.c_str() );
   if (! outputStream.good() )
   {
       printf( "\tError in opening output file: %s\n",outputFile.c_str() );
       exit ( 1 );
   }
   
   for (int i=0; i<siteRow.size(); i++)
   {
      string   siteName = siteNames[i];
      sprintf( temp_char,"%9s\0",siteName.c_str() );

      string temp_str = string ( temp_char );
      string  lineStr = string ( temp_str );


      temp_str = siteName + string ( ".dly" );
      sprintf( temp_char,"%12s\0", temp_str.c_str() );

      temp_str = string ( temp_char );
      lineStr.append ( temp_str );

      lineStr.append( "\n" );
      outputStream.write( lineStr.c_str(), lineStr.size() );
   }

   outputStream.close ();

   printf( "\tFinished creating file: %s\n", outputFile.c_str() );
}


/***************************************************
* Write duplicate set of daily files in daily_dup. *
* New daily files will have duplicated data for    *
* the previous year.                               * 
/***************************************************/
void writeDuplicateDailyFile ( string outputDir ) 
{
   string            inputFile;
   std::ifstream     imageStream;             //stream to read input EPIC daily 

   string            outputFile;
   std::ofstream     outputStream;           //output text file stream

   string            lineStr;
   char              tmp_chars[100];
   int               num; 


   printf ( "\nWriting duplicated daily weather files in daily_dup directory...\n" );


   for (int i=0; i<siteRow.size(); i++)
   {
      vector<string>  dataLines;
      num = 0;

      string   siteName = siteNames[i];
      

      //output file
      outputFile  = outputDir + siteName + string (".dly");  //EPIC daily weather file
      FileExists(outputFile.c_str(), 3 );  //the file has to be new

      outputStream.open( outputFile.c_str() );
      if (! outputStream.good() )
      {
         printf( "\tError in opening output file: %s\n",outputFile.c_str() );
         exit ( 1 );
      }
 

      //get daily float pointer vector for i site
      vector<float*> siteDailyData_get = siteDailyDataV[i];


      //output previous m=0 and current year m=1
      for ( int m=0; m<2; m++ )
      {
         for (int j=0; j<siteDailyData_get.size(); j++ )
         {
            //get day data int YYYY, MM, DD
            int *dayData = dayDataV[j];
            
            //get current time step float array pointer
            float *dailyV = siteDailyData_get[j];


            int yearVal = dayData[0];
            sprintf(tmp_chars, "%d\0", yearVal );
            string  yearStr =  string ( tmp_chars );

            int monVal = dayData[1];
            string monStr = convert2NumsTo2Chars ( monVal );  //get two digits month

            int dayVal = dayData[2];      
            string dayStr = convert2NumsTo2Chars ( dayVal );  //get two digits day

            string dateStr = yearStr + monStr + dayStr;

            string dateStr_julian = getJulianDayStr ( dateStr );
            string julianStr = dateStr_julian.substr ( 4, 3 );

            int julianDays = atoi ( julianStr.c_str() );

            
            //for previous year, skip last day of leap year
            if ( m == 0 && julianDays != 366 )    
            {
               yearVal = yearVal - 1;   //previous year

               stringstream out;
               out << yearVal;
               string yearStr_pre = out.str();

               //get previous year date 
               string dateStr_pre_julian =  yearStr_pre + julianStr;
               string dateStr_pre = getDateFromJulian ( dateStr_pre_julian );

               monStr = dateStr_pre.substr (4, 2);
               dayStr = dateStr_pre.substr (6, 2);
  
               monVal = atoi ( monStr.c_str() );
               dayVal = atoi ( dayStr.c_str() );
            }


            if ( ! (m == 0 && julianDays == 366) ) 
            {

               sprintf(tmp_chars, "%6d%4d%4d\0", yearVal,monVal,dayVal );   //convert to EPIC format
               string dataLine = string (tmp_chars );


               //convert data variable items to EPIC format
               if (NDepSelect == 0 )
               {
                  sprintf(tmp_chars, "%6.2f%6.2f%6.2f%6.2f%6.2f%6.2f%7.2f%7.2f%7.2f%7.2f%7.2f\0", dailyV[0],dailyV[1],dailyV[2],
                       dailyV[4],dailyV[5],dailyV[6],0.0,0.0,0.0,0.0,0.0 );   //convert to EPIC format
               }
               else if ( NDepSelect == 1)
               {
                  float precipVal = dailyV[4];
                  float woDEP = precipVal * wetDepR;  //convert mm P to cm P for g-N/ha/dy ratio unit

                  sprintf(tmp_chars, "%6.2f%6.2f%6.2f%6.2f%6.2f%6.2f%7.2f%7.2f%7.2f%7.2f%7.2f\0", dailyV[0],dailyV[1],dailyV[2],
                       dailyV[4],dailyV[5],dailyV[6],0.0,0.0,woDEP,0.0,0.0 );   //convert to EPIC format
               
               }
               else
               {
                  sprintf(tmp_chars, "%6.2f%6.2f%6.2f%6.2f%6.2f%6.2f%7.2f%7.2f%7.2f%7.2f%7.2f\0", dailyV[0],dailyV[1],dailyV[2],
                          dailyV[4],dailyV[5],dailyV[6],dailyV[7],dailyV[8],dailyV[9],dailyV[10],dailyV[11]);   //convert to EPIC format
  
               }

               dataLine.append ( tmp_chars );     
               dataLine.append ( "\n" );
               outputStream.write( dataLine.c_str(), dataLine.size() );
            

               //copy day 365 to 366 if previoud year is leap year          
               if ( m == 0 && julianDays == 365 && IsLeapYear ( yearVal ) )
               {

                  //store previous data part
                  string dataPart = string ( tmp_chars );

                  sprintf(tmp_chars, "%6d%4d%4d\0", yearVal,monVal,dayVal+1 );   //convert to EPIC format
                  dataLine = string (tmp_chars );                                //for output previous yeardate

                  dataLine.append (  dataPart );          //add data part
                  dataLine.append ( "\n" );
                  outputStream.write( dataLine.c_str(), dataLine.size() );

               } //repeat last for a leap previous year

            }  //skip 366 day for non leap previous year

            if ( m == 1 )
            {
               //end of reading allocated data and free it
               free ( dailyV );
            }

         } //j - day loop 

      }  // m = 2 duplicate year

      outputStream.close ();

   }   //i - site loop


   for(int i=0; i<dayDataV.size(); i++ )
   {
      int *dayData = dayDataV[i];

      free ( dayData );
   }


   printf( "\tFinished creating new duplicated year daily files under daily_dup in output directory.\n" );

}

/**************************************************************************************/