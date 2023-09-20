
from primihub.utils.logger_util import logger
from primihub.context import Context
import random
import time
class Example:
  def run(self):
    logger.info(f"request id: {Context.request_id()}")
    logger.info(f"party name: {Context.party_name()}")
    import ph_secure_lib as ph_slib
    logger.info(f"dir: {dir(ph_slib)}")
    mpc_executor = ph_slib.MPCExecutor(Context.message)
    for loop in range(1):
      data_len = 10
      # input_data = [random.randint(10, 50) for i in range(5)]
      input_data = [i+1 for i in range(data_len)]
      col_rows = [2 for i in range(data_len)]
      logger.info(f"xxxxxxxxxxxavg {loop}origin_data: {input_data}")
      result = mpc_executor.avg(input_data, col_rows)
      logger.info(f"xxxxxxxxxxxavg {loop}result_data {result}")
      logger.info("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$")
      # # time.sleep(1)
      logger.info(f"xxxxxxxxxxxsum {loop}origin_data: {input_data}")
      result = mpc_executor.sum(input_data)
      logger.info(f"xxxxxxxxxxxsum {loop}result_data {result}")
      logger.info("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$")
      # time.sleep(1)
      logger.info(f"xxxxxxxxxxxmax {loop}origin_data: {input_data}")
      result = mpc_executor.max(input_data)
      logger.info(f"xxxxxxxxxxxmax {loop}result_data {result}")
      logger.info("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$")
      # time.sleep(1)
      logger.info(f"xxxxxxxxxxxmin {loop}origin_data: {input_data}")
      result = mpc_executor.min(input_data)
      logger.info(f"xxxxxxxxxxxmin {loop}result_data {result}")
      logger.info("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$")

example = Example()
example.run()