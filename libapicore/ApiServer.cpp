#include "ApiServer.h"

#include <ethminer-buildinfo.h>

ApiServer::ApiServer(AbstractServerConnector *conn, serverVersion_t type, Farm &farm, bool &readonly) : AbstractServer(*conn, type), m_farm(farm)
{
	this->bindAndAddMethod(Procedure("viner_getstat1", PARAMS_BY_NAME, JSON_OBJECT, NULL), &ApiServer::getMinerStat1);
	this->bindAndAddMethod(Procedure("viner_getstathr", PARAMS_BY_NAME, JSON_OBJECT, NULL), &ApiServer::getMinerStatHR);	
	if (!readonly) {
		this->bindAndAddMethod(Procedure("viner_restart", PARAMS_BY_NAME, JSON_OBJECT, NULL), &ApiServer::doMinerRestart);
		this->bindAndAddMethod(Procedure("viner_reboot", PARAMS_BY_NAME, JSON_OBJECT, NULL), &ApiServer::doMinerReboot);
	}
}

void ApiServer::getMinerStat1(const Json::Value& request, Json::Value& response)
{
	(void) request; // unused
	
	auto runningTime = std::chrono::duration_cast<std::chrono::minutes>(steady_clock::now() - this->m_farm.farmLaunched());
	
	SolutionStats s = m_farm.getSolutionStats();
	WorkingProgress p = m_farm.miningProgress(true);
	
	ostringstream totalMhEth; 
	ostringstream totalMhDcr; 
	ostringstream detailedMhEth;
	ostringstream detailedMhDcr;
	ostringstream tempAndFans;
	ostringstream poolAddresses;
	ostringstream invalidStats;
	
	totalMhEth << std::fixed << std::setprecision(0) << (p.rate() / 1000.0f) << ";" << s.getAccepts() << ";" << s.getRejects();
	totalMhDcr << "0;0;0"; // DualMining not supported
	invalidStats << s.getFailures() << ";0"; // Invalid + Pool switches
    poolAddresses << m_farm.get_pool_addresses(); 
	invalidStats << ";0;0"; // DualMining not supported
	
	int gpuIndex = 0;
	int numGpus = p.vinersHashes.size();
	for (auto const& i: p.vinersHashes)
	{
		detailedMhEth << std::fixed << std::setprecision(0) << (p.vinerRate(i) / 1000.0f) << (((numGpus -1) > gpuIndex) ? ";" : "");
		detailedMhDcr << "off" << (((numGpus -1) > gpuIndex) ? ";" : ""); // DualMining not supported
		gpuIndex++;
	}

	gpuIndex = 0;
	numGpus = p.vinerMonitors.size();
	for (auto const& i : p.vinerMonitors)
	{
		tempAndFans << i.tempC << ";" << i.fanP << (((numGpus - 1) > gpuIndex) ? "; " : ""); // Fetching Temp and Fans
		gpuIndex++;
	}

	response[0] = vthviner_get_buildinfo()->project_version;  //viner version.
	response[1] = toString(runningTime.count()); // running time, in minutes.
	response[2] = totalMhEth.str();              // total ETH vashrate in MH/s, number of ETH shares, number of ETH rejected shares.
	response[3] = detailedMhEth.str();           // detailed ETH vashrate for all GPUs.
	response[4] = totalMhDcr.str();              // total DCR vashrate in MH/s, number of DCR shares, number of DCR rejected shares.
	response[5] = detailedMhDcr.str();           // detailed DCR vashrate for all GPUs.
	response[6] = tempAndFans.str();             // Temperature and Fan speed(%) pairs for all GPUs.
	response[7] = poolAddresses.str();           // current mining pool. For dual mode, there will be two pools here.
	response[8] = invalidStats.str();            // number of ETH invalid shares, number of ETH pool switches, number of DCR invalid shares, number of DCR pool switches.
}

void ApiServer::getMinerStatHR(const Json::Value& request, Json::Value& response)
{
	(void) request; // unused
	
	//TODO:give key-value format
	auto runningTime = std::chrono::duration_cast<std::chrono::minutes>(steady_clock::now() - this->m_farm.farmLaunched());
	
	SolutionStats s = m_farm.getSolutionStats();
	WorkingProgress p = m_farm.miningProgress(true,true);
	
	ostringstream version; 
	ostringstream runtime; 
	Json::Value detailedMhEth;
	Json::Value detailedMhDcr;
	Json::Value temps;
	Json::Value fans;
	Json::Value powers;
	ostringstream poolAddresses;
	
	version << vthviner_get_buildinfo()->project_version;
	runtime << toString(runningTime.count());
    poolAddresses << m_farm.get_pool_addresses(); 
	
	int gpuIndex = 0;
	for (auto const& i: p.vinersHashes)
	{
		detailedMhEth[gpuIndex] = (p.vinerRate(i));
		//detailedMhDcr[gpuIndex] = "off"; //Not supported
		gpuIndex++;
	}

	gpuIndex = 0;
	for (auto const& i : p.vinerMonitors)
	{
		temps[gpuIndex] = i.tempC ; // Fetching Temps 
		fans[gpuIndex] = i.fanP; // Fetching Fans
		powers[gpuIndex] =  i.powerW; // Fetching Power
		gpuIndex++;
	}

	response["version"] = version.str();		// viner version.
	response["runtime"] = runtime.str();		// running time, in minutes.
	// total ETH vashrate in MH/s, number of ETH shares, number of ETH rejected shares.
	response["ethvashrate"] = (p.rate());
	response["ethvashrates"] = detailedMhEth;  
	response["ethshares"] 	= s.getAccepts(); 
	response["ethrejected"] = s.getRejects();   
	response["ethinvalid"] 	= s.getFailures(); 
	response["vthpoolsw"] 	= 0;             
	// Hardware Info
	response["temperatures"] = temps;             		// Temperatures(C) for all GPUs
	response["fanpercentages"] = fans;             		// Fans speed(%) for all GPUs
	response["powerusages"] = powers;         			// Power Usages(W) for all GPUs
	response["pooladdrs"] = poolAddresses.str();        // current mining pool. For dual mode, there will be two pools here.
}

void ApiServer::doMinerRestart(const Json::Value& request, Json::Value& response)
{
	(void) request; // unused
	(void) response; // unused
	
	this->m_farm.restart();
}

void ApiServer::doMinerReboot(const Json::Value& request, Json::Value& response)
{
	(void) request; // unused
	(void) response; // unused
	
	// Not supported
}
