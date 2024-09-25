#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

import logging

# Create a logger object
logger = logging.getLogger(__name__)
logger.propagate = False

# Create a console handler and set its logging level to DEBUG
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.DEBUG)

# Create a formatter and add it to the console handler
formatter = logging.Formatter('%(levelname)s - %(message)s')
console_handler.setFormatter(formatter)

# Add the console handler to the logger
logger.addHandler(console_handler)

def set_logging_level(user_input: str) -> None:
    """Set logging level based on API user input

    Args:
        user_input (str): logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
    """

    if user_input.upper() == "DEBUG":
        logger.setLevel(logging.DEBUG)
        console_handler.setLevel(logging.DEBUG)
    elif user_input.upper() == "INFO":
        logger.setLevel(logging.INFO)
        console_handler.setLevel(logging.INFO)
    elif user_input.upper() == "WARNING":
        logger.setLevel(logging.WARNING)
        console_handler.setLevel(logging.WARNING)
    elif user_input.upper() == "ERROR":
        logger.setLevel(logging.ERROR)
        console_handler.setLevel(logging.ERROR)
    elif user_input.upper() == "CRITICAL":
        logger.setLevel(logging.CRITICAL)
        console_handler.setLevel(logging.CRITICAL)
    else:
        print("Invalid logging level entered. Using default level of INFO.")